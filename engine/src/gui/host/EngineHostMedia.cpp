// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

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

namespace hum {

juce::File EngineHost::documentDir() const {
    if (docPath_.empty()) return {};
    return juce::File(juce::String(juce::CharPointer_UTF8(docPath_.c_str()))).getParentDirectory();
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

void EngineHost::onFileNodeChanged(const std::string& organism, const std::string& text,
                                   bool sourceMoved, const std::string& fileParam) {
    Organism* c = graph_ ? graph_->find(organism) : nullptr;
    const auto* cm = model_.byName(organism);
    const auto path = cm != nullptr ? banks::resolve(text, cm->displayClass) : text;
    if (auto* fl = dynamic_cast<FileLoader*>(c)) fl->loadFromFile(path);
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
    autoDetectDeckGrid(organism, path);
}

void EngineHost::autoDetectDeckGrid(const std::string& organism, const std::string& filePath) {
    if (filePath.empty()) return;
    const auto* cm = model_.byName(organism);
    if (cm == nullptr || !classHasRole(cm->classRaw, role::kDeck)) return;
    juce::AudioBuffer<float> tmp;
    double sr = sampleRate_;
    if (!loadSoundFile(filePath, tmp, sr)) return;
    const auto est = detectBeat(tmp, sr);
    if (est.confidence > 0.0) {
        setParam(organism, "BPM", est.bpm);
        setParam(organism, "GridOffset", (double) est.offsetSamples);
    }
    const auto key = detectKey(tmp, sr);
    if (key.pitchClass >= 0)
        setParamText(organism, "Key", key.name + "  " + key.camelot);
}

}
