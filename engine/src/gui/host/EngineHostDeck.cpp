// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <cstdint>
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
void DeckHost::jumpHotCue(const std::string& name, const DeckParams& p, int index) {
    if (index < 1 || index > 8) return;
    const double v = host_.liveParamValue(name, p.hotCue(index));
    if (v > 0.0) seek(name, (std::int64_t) v);
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
