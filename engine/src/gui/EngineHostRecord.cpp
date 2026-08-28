#include "core/AppPaths.h"
#include "gui/EngineHostRecord.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include "core/AudioGraph.h"
#include "core/PerfBox.h"
#include "core/RecordTake.h"
#include "gui/EngineHost.h"
#include "io/WavWriter.h"
#include "hum/Capabilities.h"

namespace hum {

bool RecordHost::armed() const { return host_.automation().isMasterRecord(); }

void RecordHost::setCapturing(bool on) {
    if (on && !host_.capturing_) host_.beginCapturePass();
    host_.capturing_ = on;
}
bool RecordHost::capturing() const { return host_.capturing_; }

void EngineHost::trimPerfRings() {
    const double spb = (tempo() > 0.0 ? 60.0 / tempo() : 0.5) * sampleRate_;
    if (spb <= 0.0) return;
    const double keepBeats = kRingSeconds * sampleRate_ / spb;
    const double cutoff = positionBeats() - keepBeats;
    if (cutoff <= 0.0) return;
    auto drop = [&](auto& ring) {
        ring.erase(std::remove_if(ring.begin(), ring.end(),
                   [&](const auto& e) { return e.beat < cutoff; }), ring.end());
    };
    drop(perfRing_);
}

std::string EngineHost::audioCaptureTarget() const {
    auto isAudioRow = [this](const std::string& n) {
        auto* g = graph_.get();
        return g != nullptr && dynamic_cast<ClipRecorder*>(g->find(n)) != nullptr;
    };
    if (noteCaptureHint) {
        const auto hinted = noteCaptureHint();
        if (!hinted.empty() && isAudioRow(hinted)) return hinted;
    }
    std::string only;
    for (const auto& cm : model_.organisms) {
        if (!isAudioRow(cm.name)) continue;
        if (!only.empty()) return {};
        only = cm.name;
    }
    return only;
}

bool EngineHost::commitRetroactiveAudio(double startBeat, double nowBeat, double beats) {
    const auto node = audioCaptureTarget();
    if (node.empty()) return false;

    const double spb = (tempo() > 0.0 ? 60.0 / tempo() : 0.5) * sampleRate_;
    const int rs = (int) perfAudioL_.size();
    const std::int64_t head = perfAudioWrite_.load(std::memory_order_relaxed);
    const std::int64_t want = (std::int64_t) std::llround(beats * spb);
    const std::int64_t n = std::min<std::int64_t>(want, std::min<std::int64_t>(head, rs));
    if (n <= 0) return false;

    std::vector<std::vector<float>> pcm(2, std::vector<float>((size_t) n));
    for (std::int64_t i = 0; i < n; ++i) {
        const int idx = (int) ((head - n + i) % rs);
        pcm[0][(size_t) i] = perfAudioL_[(size_t) idx];
        pcm[1][(size_t) i] = perfAudioR_[(size_t) idx];
    }

    const auto dir = record_.recordingsDir();
    dir.createDirectory();
    std::vector<std::string> existing;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        existing.push_back(f.getFileName().toStdString());
    const auto name = rec::nextTakeName(existing, node + "-kept");
    const auto path = dir.getChildFile(juce::String(name)).getFullPathName().toStdString();
    if (!writeWav(path, pcm, sampleRate_)) return false;

    const int startTick = (int) std::llround(juce::jmax(0.0, nowBeat - beats)
                                             * Pattern::kTicksPerBeat);
    const int lenTicks = std::max(1, (int) std::llround(beats * Pattern::kTicksPerBeat));
    (void) startBeat;
    clips().addAudio(node, startTick, lenTicks, path);
    return true;
}

void EngineHost::keepLast(int bars) {
    if (!graph_ || bars <= 0) return;
    const double barsBeats = (double) bars * juce::jmax(1, automation_.timeSigNumerator());
    const double nowBeat = positionBeats();
    const double startBeat = juce::jmax(0.0, nowBeat - barsBeats);

    beginTransaction();
    pushUndo();

    {
        std::set<std::pair<std::string, std::string>> touched;
        for (const auto& g : perfRing_)
            if (g.beat >= startBeat) touched.insert({g.organism, g.param});
        for (const auto& [org, param] : touched)
            if (auto* c = const_cast<OrganismModel*>(model_.byName(org)))
                for (auto& l : c->automation)
                    if (l.propertyName == param)
                        l.points.erase(std::remove_if(l.points.begin(), l.points.end(),
                                       [&](const AutomationBreakpoint& b) {
                                           return b.beat >= startBeat && b.beat <= nowBeat;
                                       }), l.points.end());
    }
    for (const auto& g : perfRing_)
        if (g.beat >= startBeat)
            capturePointAt(g.organism, g.param, g.value, g.valueHi, g.isRange, g.beat);
    {
        std::set<std::string> orgs;
        for (const auto& g : perfRing_) if (g.beat >= startBeat) orgs.insert(g.organism);
        for (const auto& org : orgs)
            perfbox::addSpan(model_.perfBoxes, org, startBeat, nowBeat);
    }
    syncAutomation();

    commitRetroactiveAudio(startBeat, nowBeat, barsBeats);

    endTransaction();
    if (record_.onSessionEnded) record_.onSessionEnded();
}

void RecordHost::setArmed(bool on) { host_.automation().setMasterRecord(on); }

juce::File RecordHost::recordingsDir() const {
    const auto root = userLibraryRoot().getChildFile("Recordings");
    const auto project = host_.docPath_.empty()
        ? juce::String("Untitled")
        : juce::File(juce::String(host_.docPath_)).getFileNameWithoutExtension();
    return root.getChildFile(project);
}

void RecordHost::toggle() {
    if (armed()) {
        if (sessionActive_) endSession();
        setArmed(false);
    } else {
        setArmed(true);
        if (host_.playing_) beginSession();
        else if (onStartRolling) onStartRolling();
    }
}

void RecordHost::captureToggle() {
    if (host_.capturing_ || sessionActive_ || armed()) {
        if (sessionActive_) endSession();
        host_.endCapturePasses();
        host_.capturing_ = false;
        setArmed(false);
        return;
    }
    host_.capturing_ = true;
    host_.beginCapturePass();
    setArmed(true);
    if (host_.playing_) beginSession();
    else if (onStartRolling) onStartRolling();
}

void RecordHost::onPlay() {
    if (armed() && !sessionActive_) beginSession();
}

void RecordHost::onStop() {
    if (sessionActive_) endSession();
}

std::string EngineHost::noteCaptureTarget() const {
    auto isNoteNode = [this](const std::string& n) {
        auto* g = graph_.get();
        if (g == nullptr) return false;
        auto* live = g->find(n);
        return dynamic_cast<ClipArrangement*>(live) != nullptr
               && dynamic_cast<ClipRecorder*>(live) == nullptr;
    };
    if (noteCaptureHint) {
        const auto hinted = noteCaptureHint();
        if (!hinted.empty() && isNoteNode(hinted)) return hinted;
    }
    std::string only;
    for (const auto& cm : model_.organisms) {
        if (!isNoteNode(cm.name)) continue;
        if (!only.empty()) return {};
        only = cm.name;
    }
    return only;
}

void RecordHost::beginSession() {
    sessionActive_ = true;
    takes_.clear();
    host_.beginTransaction();
    host_.pushUndo();

    auto* graph = host_.graph_.get();
    if (!graph) return;
    const auto dir = recordingsDir();
    dir.createDirectory();
    std::vector<std::string> existing;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        existing.push_back(f.getFileName().toStdString());

    for (const auto& node : host_.arrangeableNodes()) {
        auto* cr = dynamic_cast<ClipRecorder*>(graph->find(node));
        if (!cr) continue;
        const auto* cm = host_.model_.byName(node);
        bool rec = false;
        if (cm) for (const auto& p : cm->properties)
            if (p.name == "Record") { rec = p.value >= 0.5; break; }
        if (!rec) continue;
        const auto name = rec::nextTakeName(existing, node);
        const auto path = dir.getChildFile(juce::String(name)).getFullPathName().toStdString();
        existing.push_back(name);
        {
            const juce::ScopedLock sl(host_.lock_);
            if (cr->startTake(path, host_.sampleRate_)) takes_.push_back({node, path});
        }
    }

    autoArmed_.clear();
    if (!host_.midi().anyRecordTarget())
        if (const auto note = host_.noteCaptureTarget(); !note.empty()) {
            host_.midi().setRecordTarget(note, true);
            autoArmed_.push_back(note);
        }
}

void RecordHost::endSession() {
    sessionActive_ = false;
    auto* graph = host_.graph_.get();
    const double spb = (host_.tempo() > 0.0 ? 60.0 / host_.tempo() : 0.5) * host_.sampleRate_;

    for (const auto& take : takes_) {
        auto* cr = graph ? dynamic_cast<ClipRecorder*>(graph->find(take.node)) : nullptr;
        if (!cr) continue;
        double startBeat = 0.0;
        std::int64_t len = 0;
        double lapBeats[ClipRecorder::kMaxLaps];
        std::int64_t lapSamples[ClipRecorder::kMaxLaps];
        int nLaps = 0;
        {
            const juce::ScopedLock sl(host_.lock_);
            cr->stopTake();
            startBeat = cr->takeStartBeat();
            len = cr->takeLengthSamples();
            nLaps = cr->takeLaps(lapBeats, lapSamples, ClipRecorder::kMaxLaps);
        }
        if (startBeat < 0.0 || len <= 0) continue;
        for (const auto& lap : rec::splitLaps(startBeat, len, lapBeats, lapSamples, nLaps)) {
            const int startTick = (int) std::llround(lap.startBeat * Pattern::kTicksPerBeat);
            const int lenTicks = std::max(1, rec::ticksFromSamples(lap.lengthSamples, spb));
            host_.clips().addAudio(take.node, startTick, lenTicks, take.path, lap.offsetSamples);
        }
    }
    takes_.clear();

    host_.flushMidiRecording(true);
    for (const auto& n : autoArmed_) host_.midi().setRecordTarget(n, false);
    autoArmed_.clear();
    host_.endCapturePasses();
    host_.capturing_ = false;
    host_.endTransaction();
    if (onSessionEnded) onSessionEnded();
}

}
