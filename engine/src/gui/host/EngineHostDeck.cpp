// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "hum/caps/Files.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "core/library/BankLibrary.h"
#include "core/analysis/BeatDetector.h"
#include "hum/dsp/BeatGrid.h"
#include "core/analysis/KeyDetector.h"
#include "core/packs/Roles.h"

#include "hum/dsp/DspMath.h"

namespace hum {

std::int64_t DeckHost::position(const std::string& name) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            return d->playbackPositionSamples();
    return 0;
}

std::int64_t DeckHost::length(const std::string& name) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            return d->fileLengthSamples();
    return 0;
}

double DeckHost::sampleRate(const std::string& name) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            return d->playbackSampleRate();
    return audio_.sampleRate();
}

double DeckHost::effectiveBpm(const std::string& name) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            return d->effectiveBpm();
    return 0.0;
}

void DeckHost::seek(const std::string& name, std::int64_t sample) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            d->requestSeekSamples(sample);
}

void DeckHost::setBend(const std::string& name, double percent) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            d->setBendPercent(percent);
}

void DeckHost::setScrub(const std::string& name, bool active, double targetSample) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            d->setScrub(active, targetSample);
}

std::vector<float> DeckHost::waveform(const std::string& name) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            return d->waveformPeaks();
    return {};
}

std::vector<float> DeckHost::waveBetween(const std::string& name, std::int64_t from,
                                         std::int64_t to, int buckets) {
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name)))
            return d->wavePeaksBetween(from, to, buckets);
    return {};
}

std::int64_t DeckHost::quantizeToGrid(const std::string& name, const DeckParams& p, std::int64_t sample) {
    if (host_.liveParamValue(name, p.quantize) < 0.5) return sample;
    BeatGrid g{std::max(1.0, host_.liveParamValue(name, p.bpm)),
               (std::int64_t) host_.liveParamValue(name, p.gridOffset)};
    const double sr = sampleRate(name);
    return (std::int64_t) std::max(0.0, g.nearestBeatSample((double) sample, sr));
}

void DeckHost::setCue(const std::string& name, const DeckParams& p) {
    host_.setParam(name, p.cue, (double) quantizeToGrid(name, p, position(name)));
}
void DeckHost::jumpCue(const std::string& name, const DeckParams& p) {
    seek(name, (std::int64_t) host_.liveParamValue(name, p.cue));
}
void DeckHost::setHotCue(const std::string& name, const DeckParams& p, int index) {
    if (index >= 1 && index <= 8)
        host_.setParam(name, p.hotCue(index), (double) quantizeToGrid(name, p, position(name)));
}
void DeckHost::startPlaying(const std::string& name) {
    if (audio_.graph() == nullptr) return;
    auto* cap = dynamic_cast<FileTransportCap*>(audio_.graph()->find(name));
    if (cap == nullptr) return;
    const auto play = cap->playSwitch();
    if (!play.empty() && host_.liveParamValue(name, play) < 0.5) host_.setParam(name, play, 1.0);
}
void DeckHost::jumpHotCue(const std::string& name, const DeckParams& p, int index) {
    if (index < 1 || index > 8) return;
    const double v = host_.liveParamValue(name, p.hotCue(index));
    if (v < 0.0) return;
    seek(name, (std::int64_t) v);
    startPlaying(name);
}
void DeckHost::nudgeHotCue(const std::string& name, const DeckParams& p, double way) {
    const auto it = lastPad_.find(name);
    if (it == lastPad_.end()) return;
    const auto cue = p.hotCue(it->second);
    const double at = host_.liveParamValue(name, cue);
    if (at < 0.0) return;
    const double step = nudgeStepSeconds((int) host_.liveParamValue(name, "NudgeStep"));
    host_.setParam(name, cue, std::max(0.0, at + way * step * sampleRate(name)));
}
void DeckHost::clearHotCue(const std::string& name, const DeckParams& p, int index) {
    if (index >= 1 && index <= 8) host_.setParam(name, p.hotCue(index), -1.0);
}
void DeckHost::setBeatLoop(const std::string& name, const DeckParams& p, double beats) {
    const double bpm = std::max(1.0, host_.liveParamValue(name, p.bpm));
    const double spb = kSecondsPerMinute / bpm * sampleRate(name);
    const std::int64_t in = quantizeToGrid(name, p, position(name));
    host_.setParam(name, p.loopBeats, beats);
    host_.setParam(name, p.loopIn, (double) in);
    host_.setParam(name, p.loopOut, (double) in + beats * spb);
    host_.setParam(name, p.loop, 1.0);
    startPlaying(name);
}
void DeckHost::scaleBeatLoop(const std::string& name, const DeckParams& p, double factor) {
    const double beats = juce::jlimit(0.0625, 64.0, host_.liveParamValue(name, p.loopBeats) * factor);
    const double in = host_.liveParamValue(name, p.loopIn);
    const double spb = kSecondsPerMinute / std::max(1.0, host_.liveParamValue(name, p.bpm)) * sampleRate(name);
    host_.setParam(name, p.loopBeats, beats);
    host_.setParam(name, p.loopOut, in + beats * spb);
}
void DeckHost::toggleLoop(const std::string& name, const DeckParams& p) {
    host_.setParam(name, p.loop, host_.liveParamValue(name, p.loop) >= 0.5 ? 0.0 : 1.0);
}
namespace {

DeckParams schemaParams() {
    DeckParams p;
    p.bpm = "BPM";
    p.cue = "Cue";
    p.gridOffset = "GridOffset";
    p.hotCuePrefix = "HotCue_";
    p.loop = "Loop";
    p.loopBeats = "LoopBeats";
    p.loopIn = "LoopIn";
    p.loopOut = "LoopOut";
    p.quantize = "Quantize";
    return p;
}

struct NudgeTrigger { const char* param; double way; };

const NudgeTrigger kCueNudges[] = {
    {"CueNudgeBack", -1.0},
    {"CueNudgeForward", 1.0},
};

struct BendTrigger { const char* param; double way; };

const BendTrigger kBends[] = {
    {"BendUp", 1.0},
    {"BendDown", -1.0},
};

struct BeatTrigger { const char* param; double beats; };

const BeatTrigger kBeatLoops[] = {
    {"BeatLoop_1", 1.0}, {"BeatLoop_4", 4.0}, {"BeatLoop_8", 8.0},
};

const BeatTrigger kRolls[] = {
    {"Roll_1_8", 0.125}, {"Roll_1_4", 0.25}, {"Roll_1_2", 0.5}, {"Roll_1", 1.0},
};

}

bool DeckHost::fireTrigger(const std::string& name, const std::string& param, double value) {
    if (audio_.graph() == nullptr) return false;
    if (dynamic_cast<DeckControl*>(audio_.graph()->find(name)) == nullptr) return false;

    for (const auto& r : kRolls) {
        if (param != r.param) continue;
        const auto p = schemaParams();
        if (value >= 0.5) beginLoopRoll(name, p, r.beats);
        else endLoopRoll(name, p);
        return true;
    }
    for (const auto& b : kBends) {
        if (param != b.param) continue;
        setBend(name, value >= 0.5 ? b.way * kBendPercent : 0.0);
        return true;
    }
    if (value < 0.5) return param.rfind("Pad_", 0) == 0 || param == "PadClear"
                         || param == "PadClearAll"
                         || param == "CueNudgeBack" || param == "CueNudgeForward"
                         || param == "CueSet" || param == "CueJump"
                         || param == "LoopHalve" || param == "LoopDouble"
                         || param == "BeatLoop_1" || param == "BeatLoop_4" || param == "BeatLoop_8";

    const auto p = schemaParams();
    if (param.rfind("Pad_", 0) == 0) {
        const int index = std::atoi(param.c_str() + 4);
        if (index < 1 || index > 8) return false;
        lastPad_[name] = index;
        if (host_.liveParamValue(name, p.hotCue(index)) < 0.0) setHotCue(name, p, index);
        else jumpHotCue(name, p, index);
        return true;
    }
    if (param == "PadClear") {
        const auto it = lastPad_.find(name);
        if (it != lastPad_.end()) clearHotCue(name, p, it->second);
        return true;
    }
    for (const auto& n : kCueNudges) {
        if (param != n.param) continue;
        nudgeHotCue(name, p, n.way);
        return true;
    }
    if (param == "PadClearAll") {
        for (int index = 1; index <= 8; ++index) clearHotCue(name, p, index);
        return true;
    }
    if (param == "CueSet")     { setCue(name, p); return true; }
    if (param == "CueJump")    { jumpCue(name, p); return true; }
    if (param == "LoopHalve")  { scaleBeatLoop(name, p, 0.5); return true; }
    if (param == "LoopDouble") { scaleBeatLoop(name, p, 2.0); return true; }
    for (const auto& b : kBeatLoops)
        if (param == b.param) { setBeatLoop(name, p, b.beats); return true; }
    return false;
}

void DeckHost::beginLoopRoll(const std::string& name, const DeckParams& p, double beats) {
    LiveControlHold live(doc_);
    setBeatLoop(name, p, beats);
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name))) d->beginLoopRoll();
}
void DeckHost::endLoopRoll(const std::string& name, const DeckParams& p) {
    LiveControlHold live(doc_);
    if (audio_.graph())
        if (auto* d = dynamic_cast<DeckControl*>(audio_.graph()->find(name))) d->endLoopRoll();
    host_.setParam(name, p.loop, 0.0);
}

}
