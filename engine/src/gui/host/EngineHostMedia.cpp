// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/AppPaths.h"
#include "gui/host/EngineHost.h"

#include "gui/editor/files/FilePick.h"
#include "gui/video/VideoProbe.h"
#include "hum/caps/Video.h"

#include <string>
#include <vector>

#include "core/library/BankLibrary.h"
#include "core/analysis/BeatDetector.h"
#include "core/analysis/KeyDetector.h"
#include "core/packs/Roles.h"
#include "hum/caps/Files.h"
#include "hum/caps/Params.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "io/MediaRefs.h"
#include "core/project/ProjectFolder.h"

namespace hum {

namespace {
constexpr double kGridScanSeconds = 600.0;
constexpr const char* kCompanionSound = "FileSound";

juce::AudioBuffer<float> prefixOf(const juce::AudioBuffer<float>& buf, int numSamples) {
    const int channels = buf.getNumChannels();
    if (channels <= 0) return {};
    std::vector<float*> reads((size_t) channels);
    for (int c = 0; c < channels; ++c)
        reads[(size_t) c] = const_cast<float*>(buf.getReadPointer(c));
    return juce::AudioBuffer<float>(reads.data(), channels,
                                    std::min(numSamples, buf.getNumSamples()));
}

constexpr int kLoadWaitMs = 30000;
constexpr int kLoadPollMs = 2;
}

juce::File EngineHost::documentDir() const {
    if (docPath_.empty()) return {};
    return fileAt(docPath_).getParentDirectory();
}

bool EngineHost::saveIntoProject(const std::string& chosen, std::string& landed,
                                 std::string& error) {
    const auto target = project::landingFor(chosen);
    const auto pick = fileAt(chosen);
    const auto into = fileAt(target);
    const auto folder = into.getParentDirectory().getFullPathName().toStdString();
    if (!pick.existsAsFile() && !project::mark(folder)) {
        error = "could not create " + folder;
        return false;
    }
    if (!saveFile(target, error)) return false;
    landed = target;
    return true;
}

void EngineHost::gatherRecordingsFor(const std::string& path) {
    project::Shelves shelves;
    shelves.root = project::rootOf(path);
    if (shelves.root.empty()) return;
    shelves.formerRoot = project::rootOf(docPath_);
    if (shelves.formerRoot == shelves.root) shelves.formerRoot.clear();
    shelves.sharedRecordings = userRecordingsDir().getFullPathName().toStdString();

    for (const auto& ref : media::refsOf(model_, documentDir())) {
        if (!ref.exists || !ref.resolved.existsAsFile()) continue;
        const auto source = ref.resolved.getFullPathName().toStdString();
        const auto wanted = project::gatherTarget(source, shelves);
        if (wanted.empty()) continue;
        const auto text = project::gatheredCopy(source, wanted);
        if (text.empty()) continue;
        auto* cm = mutableByName(ref.organism);
        if (cm == nullptr) continue;
        if (ref.isClip()) {
            if (ref.channel < (int) cm->pattern.channels.size())
                cm->pattern.channels[(size_t) ref.channel].audioFile = text;
        } else {
            for (auto& p : cm->properties)
                if (p.name == ref.param) p.text = text;
        }
    }
    project::pruneEmptyFolders(shelves.sharedRecordings);
}

std::vector<media::Ref> EngineHost::missingMedia() const {
    return media::missingOf(model_, documentDir());
}

int EngineHost::relocateMedia(const media::Ref& located, const juce::File& found) {
    const auto plan = media::relocationPlan(missingMedia(), located, found);
    if (plan.empty()) return 0;
    pushUndo();
    int done = 0;
    for (const auto& [ref, file] : plan) {
        const auto path = file.getFullPathName().toStdString();
        if (ref.isClip()) {
            auto* cm = mutableByName(ref.organism);
            if (cm == nullptr || ref.channel < 0 || ref.channel >= (int) cm->pattern.channels.size()) continue;
            cm->pattern.channels[(size_t) ref.channel].audioFile = path;
            dirty_ = true;
            ++changeStamp_;
            syncPattern(ref.organism);
        } else {
            setParamText(ref.organism, ref.param, path);
        }
        ++done;
    }
    return done;
}

void EngineHost::noteMediaLength(Organism* c, const std::string& path) {
    auto* span = dynamic_cast<MediaDuration*>(c);
    if (span == nullptr || dynamic_cast<VideoNode*>(c) == nullptr) return;
    const auto f = fileAt(path);
    const bool readable = isVideoFile(f) && f.existsAsFile();
    span->noteMediaSeconds(readable ? probeVideoSeconds(f) : 0.0);
}

bool EngineHost::loadInBackground(const std::string& organism, const std::string& path) {
    Organism* c = graph_ ? graph_->find(organism) : nullptr;
    auto* fl = dynamic_cast<FileLoader*>(c);
    if (fl == nullptr || dynamic_cast<LoadsOffThread*>(c) == nullptr) return false;
    if (path.empty()) return false;
    const auto* cm = model_.byName(organism);
    const bool deck = cm != nullptr && classHasRole(cm->classRaw, role::kDeck);
    {
        const juce::ScopedLock sl(loadingLock_);
        for (auto& l : loading_)
            if (l.organism == organism) {
                if (l.path == path && l.node == c) return true;
                l.again = true;
                l.path = path;
                if (auto* stop = dynamic_cast<AbortsLoad*>(c)) stop->abandonLoad();
                return true;
            }
        loading_.push_back({organism, files::fileNameOf(path), path, Loading::Stage::Reading, 0.0f,
                            juce::Time::getMillisecondCounterHiRes(), c, false});
    }
    loaders_.addJob([this, organism, path, fl, c, deck] {
        fl->loadFromFile(path);
        noteMediaLength(c, path);
        if (deck) {
            noteLoadStage(organism, Loading::Stage::Listening);
            autoDetectDeckGrid(organism, path);
        }
        finishLoad(organism, c);
    });
    return true;
}

void EngineHost::noteLoadStage(const std::string& organism, Loading::Stage stage) {
    const juce::ScopedLock sl(loadingLock_);
    for (auto& l : loading_)
        if (l.organism == organism) l.stage = stage;
}

void EngineHost::finishLoad(const std::string& organism, Organism* c) {
    std::string fault;
    if (auto* f = dynamic_cast<LoadFault*>(c)) fault = f->loadFault();
    std::string file, again;
    {
        const juce::ScopedLock sl(loadingLock_);
        for (const auto& l : loading_)
            if (l.organism == organism) {
                file = l.file;
                if (l.again) again = l.path;
            }
        loading_.erase(std::remove_if(loading_.begin(), loading_.end(),
                                      [&](const Loading& l) { return l.organism == organism; }),
                       loading_.end());
    }
    if (!again.empty()) {
        scheduler_.post([this, alive = hostAlive_, organism, again] {
            if (*alive) loadInBackground(organism, again);
        });
        return;
    }
    if (!fault.empty()) noteLoadFault(organism, file, fault);
}

void EngineHost::noteLoadFault(const std::string& organism, const std::string& path,
                               const std::string& message) {
    const juce::ScopedLock sl(loadingLock_);
    for (const auto& f : faults_)
        if (f.organism == organism && f.message == message) return;
    faults_.push_back({organism, files::fileNameOf(path), message});
}

std::vector<EngineHost::LoadFaultNote> EngineHost::takeLoadFaults() {
    const juce::ScopedLock sl(loadingLock_);
    auto out = std::move(faults_);
    faults_.clear();
    return out;
}

std::vector<EngineHost::Loading> EngineHost::loadsInFlight() {
    const juce::ScopedLock sl(loadingLock_);
    auto out = loading_;
    for (auto& l : out)
        if (auto* p = dynamic_cast<LoadsOffThread*>(graph_ ? graph_->find(l.organism) : nullptr))
            l.progress = p->loadProgress();
    return out;
}

bool EngineHost::loadsOutliveGraph() const {
    const juce::ScopedLock sl(loadingLock_);
    for (const auto& l : loading_)
        if (graph_ == nullptr || graph_->find(l.organism) != l.node) return false;
    return true;
}

void EngineHost::queueDeferredLoads() {
    std::vector<std::pair<std::string, std::string>> wanted;
    for (const auto& cm : model_.organisms) {
        auto* live = graph_ ? graph_->find(cm.name) : nullptr;
        auto* off = dynamic_cast<LoadsOffThread*>(live);
        if (off == nullptr || off->loadProgress() >= 1.0f) continue;
        for (const auto& p : cm.properties)
            if (p.name.rfind("File", 0) == 0 && !p.text.empty())
                wanted.emplace_back(cm.name, banks::resolve(p.text, cm.displayClass));
    }
    for (const auto& [organism, path] : wanted) loadInBackground(organism, path);
}

void EngineHost::abandonLoads() {
    const juce::ScopedLock sl(loadingLock_);
    for (auto& l : loading_) {
        l.again = false;
        if (auto* stop = dynamic_cast<AbortsLoad*>(l.node)) stop->abandonLoad();
    }
}

void EngineHost::waitForLoads(bool abandon) {
    if (abandon) {
        const juce::ScopedLock sl(loadingLock_);
        for (const auto& l : loading_)
            if (auto* stop = dynamic_cast<AbortsLoad*>(l.node)) stop->abandonLoad();
    }
    const auto until = juce::Time::getMillisecondCounter() + (juce::uint32) kLoadWaitMs;
    while (loaders_.getNumJobs() > 0 && juce::Time::getMillisecondCounter() < until)
        juce::Thread::sleep(kLoadPollMs);
    loaders_.removeAllJobs(true, kLoadWaitMs);
    const juce::ScopedLock sl(loadingLock_);
    loading_.clear();
}

void EngineHost::forgetCompanionSound(const std::string& organism, Organism* live) {
    if (auto* cm = model_.byName(organism))
        for (auto& p : cm->properties)
            if (p.name == kCompanionSound) p.text.clear();
    const juce::ScopedLock sl(lock_);
    if (live != nullptr)
        if (auto* p = live->params.byName(kCompanionSound)) p->text.clear();
}

void EngineHost::onFileNodeChanged(const std::string& organism, const std::string& text,
                                   bool sourceMoved, const std::string& fileParam) {
    Organism* c = graph_ ? graph_->find(organism) : nullptr;
    const auto* cm = model_.byName(organism);
    if (fileParam == "File") forgetCompanionSound(organism, c);
    auto wanted = text;
    if (wanted.empty() && cm != nullptr)
        for (const auto& p : cm->properties)
            if (p.name == fileParam) wanted = p.text;
    const auto path = cm != nullptr ? banks::resolve(wanted, cm->displayClass) : wanted;
    if (!path.empty() && !fileAt(path).existsAsFile() && !fileAt(path).isDirectory())
        noteLoadFault(organism, path, "the file is missing");
    bool queued = false;
    if (auto* fl = dynamic_cast<FileLoader*>(c)) {
        queued = loadInBackground(organism, path);
        if (!queued) {
            fl->loadFromFile(path);
            noteMediaLength(c, path);
        }
    }
    if (auto* live = dynamic_cast<LiveParamRange*>(c);
        sourceMoved && live != nullptr && cm != nullptr) {
        std::vector<std::pair<std::string, double>> restarts;
        for (const auto& p : cm->properties) {
            double lo = 0.0, hi = 0.0;
            if (live->liveParamRange(p.name, lo, hi) && live->rangeFollowsFile(p.name, fileParam))
                restarts.emplace_back(p.name, lo);
        }
        for (const auto& [n, v] : restarts) setParam(organism, n, v);
    }
    pullVoiceParams(organism);
    if (!queued && cm != nullptr && classHasRole(cm->classRaw, role::kDeck))
        autoDetectDeckGrid(organism, path);
}

bool EngineHost::gridAlreadyScanned(const std::string& organism, const std::string& filePath) {
    const juce::ScopedLock sl(loadingLock_);
    auto& seen = gridScanned_[organism];
    if (seen == filePath) return true;
    seen = filePath;
    return false;
}

void EngineHost::autoDetectDeckGrid(const std::string& organism, const std::string& filePath) {
    if (filePath.empty() || gridAlreadyScanned(organism, filePath)) return;
    const auto sound = loadSharedSoundFile(filePath, {}, kGridScanSeconds);
    if (sound.audio == nullptr) return;
    const double sr = sound.info.sampleRate;
    const auto scan = prefixOf(*sound.audio, (int) std::min((double) sound.audio->getNumSamples(),
                                                            kGridScanSeconds * sr));
    const auto est = detectBeat(scan, sr);
    const auto key = detectKey(scan, sr);
    scheduler_.post([this, alive = hostAlive_, organism, est, key] {
        if (!*alive) return;
        if (graph_ == nullptr || graph_->find(organism) == nullptr) return;
        if (est.confidence > 0.0) {
            setParam(organism, "BPM", est.bpm);
            setParam(organism, "GridOffset", (double) est.offsetSamples);
        }
        if (key.pitchClass >= 0)
            setParamText(organism, "Key", key.name + "  " + key.camelot);
    });
}

}
