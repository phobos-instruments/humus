// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/LinkSync.h"

#include <chrono>
#include <cmath>

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>

namespace hum {

struct LinkSync::Impl {
    ableton::Link link{120.0};
    ableton::link::HostTimeFilter<ableton::Link::Clock> filter;
    double monotonicSampleTime = 0.0;
    double filterRate = 0.0;
};

LinkSync::LinkSync() : impl_(std::make_unique<Impl>()) {}
LinkSync::~LinkSync() = default;

void LinkSync::setEnabled(bool on) { impl_->link.enable(on); }
bool LinkSync::enabled() const { return impl_->link.isEnabled(); }
int LinkSync::numPeers() const { return (int) impl_->link.numPeers(); }

void LinkSync::proposeTempo(double bpm) {
    if (bpm <= 0.0 || !impl_->link.isEnabled()) return;
    auto state = impl_->link.captureAppSessionState();
    if (std::abs(state.tempo() - bpm) < 0.01) return;
    state.setTempo(bpm, impl_->link.clock().micros());
    impl_->link.commitAppSessionState(state);
}

void LinkSync::setStartStopSyncEnabled(bool on) { impl_->link.enableStartStopSync(on); }

bool LinkSync::sessionPlaying() const {
    if (!impl_->link.isEnabled()) return false;
    return impl_->link.captureAppSessionState().isPlaying();
}

void LinkSync::proposePlaying(bool on) {
    if (!impl_->link.isEnabled()) return;
    auto state = impl_->link.captureAppSessionState();
    if (state.isPlaying() == on) return;
    state.setIsPlaying(on, impl_->link.clock().micros());
    impl_->link.commitAppSessionState(state);
}

double LinkSync::sessionTempo() const {
    if (!impl_->link.isEnabled()) return 0.0;
    return impl_->link.captureAppSessionState().tempo();
}

LinkSync::Pulse LinkSync::capture(int numSamples, double sampleRate,
                                  int outputLatencySamples, double quantum) {
    if (!impl_->link.isEnabled() || sampleRate <= 0.0 || quantum <= 0.0) return {};
    if (impl_->filterRate != sampleRate) {
        impl_->filter.reset();
        impl_->monotonicSampleTime = 0.0;
        impl_->filterRate = sampleRate;
    }
    const auto host = impl_->filter.sampleTimeToHostTime(impl_->monotonicSampleTime);
    impl_->monotonicSampleTime += (double) numSamples;
    const auto latency = std::chrono::microseconds(
        (long long) (1.0e6 * (double) outputLatencySamples / sampleRate));
    const auto state = impl_->link.captureAudioSessionState();
    Pulse p;
    p.bpm = state.tempo();
    p.phase = state.phaseAtTime(host + latency, quantum);
    return p;
}

}
