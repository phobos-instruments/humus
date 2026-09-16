// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Follower/Follower.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum {

void Follower::reset() {
    env_.reset();
    open_ = false;
    holdLeft_ = 0;
    soundingNote_ = -1;
    pendingCount_ = 0;
    gate_.store(0.0f, std::memory_order_relaxed);
}

void Follower::emit(int offset, bool on, int velocity) {
    if (pendingCount_ >= 8) return;
    const int note = on ? std::clamp((int) params.get("Note", 60.0), 0, kMidiMax) : soundingNote_;
    if (note < 0) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) note;
    e.data[2] = (unsigned char) (on ? std::clamp(velocity, 1, kMidiMax) : 0);
    e.size = 3;
    e.sampleOffset = offset;
    pending_[pendingCount_++] = e;
    soundingNote_ = on ? note : -1;
}

int Follower::collectMidi(int, MidiEvent* out, int capacity) {
    const int n = std::min(pendingCount_, capacity);
    for (int i = 0; i < n; ++i) out[i] = pending_[i];
    pendingCount_ = 0;
    return n;
}

void Follower::process(const float* const* in, int numIn, float* const* out, int numOut,
                       int numSamples, const Transport&) {
    env_.set(std::max(0.0, params.get("Attack", 10.0)),
             std::max(0.0, params.get("Release", 200.0)), sampleRate_);
    const float gain = (float) std::max(0.0, params.get("Gain", 1.0));
    const float threshold = (float) std::clamp(params.get("Threshold", 0.1), 0.0, 1.0);
    const int holdSamples = (int) (std::max(0.0, params.get("Hold", 50.0)) * 0.001 * sampleRate_);

    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    float* dst = numOut > 0 ? out[0] : nullptr;
    float* gate = numOut > 1 ? out[1] : nullptr;
    float last = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        const float level = (float) env_.process(src ? std::abs(src[i]) : 0.0f) * gain;
        if (dst) dst[i] = level;
        last = level;
        if (!open_ && threshold > 0.0f && level >= threshold) {
            open_ = true;
            holdLeft_ = holdSamples;
            emit(i, true, (int) std::lround(std::min(level, 1.0f) * kMidiMaxF));
        } else if (open_) {
            if (holdLeft_ > 0) --holdLeft_;
            else if (level < threshold * 0.5f || threshold <= 0.0f) {
                open_ = false;
                emit(i, false, 0);
            }
        }
        if (gate) gate[i] = open_ ? 1.0f : 0.0f;
    }
    for (int c = 2; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    if (numSamples > 0) ctl_.store(std::clamp(last, 0.0f, 1.0f), std::memory_order_relaxed);
    gate_.store(open_ ? 1.0f : 0.0f, std::memory_order_relaxed);
}

}
