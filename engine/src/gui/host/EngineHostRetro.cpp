// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/AppPaths.h"
#include "gui/host/EngineHostRecord.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>
#include "core/graph/AudioGraph.h"
#include "core/graph/PerfBox.h"
#include "core/timeline/RecordTake.h"
#include "gui/host/EngineHost.h"
#include "gui/video/VideoLog.h"
#include "io/WavWriter.h"
#include "hum/caps/Files.h"
#include "hum/dsp/DspMath.h"

namespace hum {

void EngineHost::trimPerfRings() {
    const double spb = (tempo() > 0.0 ? kSecondsPerMinute / tempo() : 0.5) * sampleRate_;
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

    const double spb = (tempo() > 0.0 ? kSecondsPerMinute / tempo() : 0.5) * sampleRate_;
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
    const double nowBeat = positionBeats();
    const double barsBeats = (double) bars * automation_.meterAt(nowBeat).quarterNotesPerBar();
    const double startBeat = juce::jmax(0.0, nowBeat - barsBeats);

    beginTransaction();
    pushUndo();

    {
        std::set<std::pair<std::string, std::string>> touched;
        for (const auto& g : perfRing_)
            if (g.beat >= startBeat) touched.insert({g.organism, g.param});
        for (const auto& [org, param] : touched)
            if (auto* c = model_.byName(org))
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

}
