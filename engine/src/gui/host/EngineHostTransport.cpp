// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/packs/Categories.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "hum/CameraCapture.h"
#include "hum/SerialPort.h"
#include "core/graph/GraphIo.h"
#include "core/net/LinkChase.h"
#include "core/packs/Roles.h"
#include "gui/app/AppSettings.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"
#include "hum/caps/Osc.h"
#include "hum/dsp/DspMath.h"

namespace hum {

void EngineHost::play() {
    if (capturing_) beginCapturePass();
    if (linkEnabled_.load(std::memory_order_relaxed)) {
        linkAlign_.store(true, std::memory_order_relaxed);
        if (link_ && linkStartStop_.load(std::memory_order_relaxed)) {
            link_->proposePlaying(true);
            linkLastSessionPlaying_ = true;
        }
    }
    if (pendingCountInBars_ > 0 && !playing_ && audioRunning_) {
        const juce::ScopedLock sl(lock_);
        if (graph_) {
            const auto& t = graph_->transport();
            const auto meter = t.meterAt(t.beats());
            const double spb = t.samplesPerBeat();
            countInBar_ = meter.quarterNotesPerBar() * spb;
            countInTick_ = std::max(1.0, meter.quarterNotesPerBeat() * spb);
            countInTotal_ = (std::int64_t) (pendingCountInBars_ * countInBar_);
            pendingCountInBars_ = 0;
            countInNextBeat_ = 0.0;
            countInLeft_.store(countInTotal_, std::memory_order_release);
            return;
        }
    }
    pendingCountInBars_ = 0;
    playing_ = true;
    clearTouches();
    {
        const juce::ScopedLock sl(lock_);
        if (graph_) graph_->transport().setPlaying(true);
    }
    record_.onPlay();
    syncTransportEvent(true);
}

void EngineHost::pumpOscOut() {
    for (auto& cm : model_.organisms) {
        auto* src = dynamic_cast<OscValueSource*>(liveOrganism(cm.name));
        if (src == nullptr || !src->oscEnabled()) continue;
        OscValueSource::OscVal vals[64];
        const int n = src->oscValues(vals, 64);
        auto& last = oscOutLast_[cm.name];
        if ((int) last.size() != n) last.assign((size_t) n, -1.0e9f);
        juce::String seg;
        for (const char ch : cm.name)
            seg += (juce::CharacterFunctions::isLetterOrDigit((juce::juce_wchar) ch)
                    || ch == '_' || ch == '-') ? juce::String::charToString((juce::juce_wchar) ch)
                                               : juce::String("_");
        for (int i = 0; i < n; ++i) {
            if (std::abs(vals[i].value - last[(size_t) i]) < 1.0e-4f) continue;
            last[(size_t) i] = vals[i].value;
            osc().sendValue("/humus/" + seg + "/" + vals[i].suffix, vals[i].value);
        }
    }
}

void EngineHost::serviceCountIn() {
    if (punchFire_.exchange(false)) record_.onPunchIn();
    if (!countInFire_.exchange(false)) return;
    playing_ = true;
    clearTouches();
    record_.onPlay();
    syncTransportEvent(true);
}

void EngineHost::stop() {
    countInLeft_.store(0);
    cancelPreRoll();
    record_.clearPending();
    record_.onStop();
    finishOnDemandClips();
    playing_ = false;
    clearTouches();
    {
        const juce::ScopedLock sl(lock_);
        if (graph_) graph_->transport().setPlaying(false);
    }
    syncTransportEvent(false);
    if (link_ && linkEnabled_.load(std::memory_order_relaxed)
        && linkStartStop_.load(std::memory_order_relaxed)) {
        link_->proposePlaying(false);
        linkLastSessionPlaying_ = false;
    }
}

void EngineHost::goToStart() { setPositionBeats(0.0); }

void EngineHost::playFromStart() { goToStart(); play(); }

void EngineHost::setPositionBeats(double beat) {
    const double target = beat < 0.0 ? 0.0 : beat;
    locateBeat_ = target;
    ++locateStamp_;
    seekBeatsReq_.store(target, std::memory_order_relaxed);
    if (!graphSelfDriven()) {
        const juce::ScopedLock sl(lock_);
        takeTransportRequests();
        publishClock();
    }
    if (!playing_) applyStateAt(target);
}

void EngineHost::setTempo(double bpm) {
    if (bpm <= 0.0) return;
    model_.clock.tempo = bpm;
    if (link_ && linkEnabled_.load(std::memory_order_relaxed)) link_->proposeTempo(bpm);
    tempoReq_.store(bpm, std::memory_order_relaxed);
    if (!graphSelfDriven()) {
        const juce::ScopedLock sl(lock_);
        takeTransportRequests();
        publishClock();
    }
}

void EngineHost::performTempo(double bpm) {
    if (bpm <= 0.0) return;
    if (capturing_ || playing_) {
        setParam(clockNodeName(), kTempoParam, bpm);
        return;
    }
    setTempo(bpm);
}

void EngineHost::publishClock() {
    if (graph_ == nullptr) {
        liveBar_.store(1, std::memory_order_relaxed);
        liveBeatInBar_.store(1.0, std::memory_order_relaxed);
        liveBeats_.store(0.0, std::memory_order_relaxed);
        liveSeconds_.store(0.0, std::memory_order_relaxed);
        return;
    }
    auto& t = graph_->transport();
    liveBar_.store(t.bar(), std::memory_order_relaxed);
    liveBeatInBar_.store(t.beatInBar(), std::memory_order_relaxed);
    liveBeats_.store(t.beats(), std::memory_order_relaxed);
    liveSeconds_.store(sampleRate_ > 0.0
                           ? (double) t.samplePosition() / sampleRate_ : 0.0,
                       std::memory_order_relaxed);
}

int EngineHost::positionBar() { return liveBar_.load(std::memory_order_relaxed); }

double EngineHost::positionBeat() { return liveBeatInBar_.load(std::memory_order_relaxed); }

double EngineHost::positionBeats() { return liveBeats_.load(std::memory_order_relaxed); }

double EngineHost::positionSeconds() { return liveSeconds_.load(std::memory_order_relaxed); }

void EngineHost::advanceModulation(double dt) {
    if (mod_.map().empty() || dt <= 0.0) return;
    LiveControlScope live(*this);
    for (const auto& u : mod_.tick(dt)) setParam(u.organism, u.param, u.value);
    if (audioRunning_ || graph_ == nullptr) return;
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const juce::ScopedLock sl(lock_);
    graph_->applyModRoutes(juce::jmax(1, (int) std::lround(dt * sr)));
}

void EngineHost::takeTransportRequests() {
    if (graph_ == nullptr) return;
    if (const double b = seekBeatsReq_.exchange(-1.0, std::memory_order_relaxed); b >= 0.0)
        graph_->transport().setBeatPosition(b);
    if (const double t = tempoReq_.exchange(0.0, std::memory_order_relaxed); t > 0.0)
        graph_->transport().setTempo(t);
}

void EngineHost::setSongLengthBeats(double beats) {
    model_.clock.songLength = std::max(0.0, beats);
    dirty_ = true;
}

double EngineHost::songEndBeat() const {
    if (model_.clock.songLength > 0.0) return model_.clock.songLength;
    double end = 0.0;
    for (const auto& c : model_.organisms) {
        if (!c.pattern.present) continue;
        for (int i = 0; i < clipops::clipCount(c.pattern); ++i)
            end = std::max(end, (clipops::clipStart(c.pattern, i)
                                 + clipops::clipLength(c.pattern, i))
                                    / (double) Pattern::kTicksPerBeat);
    }
    if (model_.clock.loopEnabled) end = std::max(end, model_.clock.loopEnd);
    return end;
}

}
