// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Button/Button.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum {

void Button::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
}

void Button::reset() {
    pressed_ = false;
    on_ = false;
    primed_ = false;
    soundingNote_ = -1;
    pendingCount_ = 0;
    smoothed_ = (float) params.get("Off", 0.0);
    ctl_.store(0.0f, std::memory_order_relaxed);
}

void Button::emit(bool on) {
    if (pendingCount_ >= 2) return;
    const int note = on ? std::clamp((int) params.get("Note", 60.0), 0, kMidiMax) : soundingNote_;
    if (note < 0) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) note;
    e.data[2] = (unsigned char) (on ? std::clamp((int) params.get("Velocity", 100.0), 1, kMidiMax) : 0);
    e.size = 3;
    e.sampleOffset = 0;
    pending_[pendingCount_++] = e;
    soundingNote_ = on ? note : -1;
}

int Button::collectMidi(int, MidiEvent* out, int capacity) {
    const int n = std::min(pendingCount_, capacity);
    for (int i = 0; i < n; ++i) out[i] = pending_[i];
    pendingCount_ = 0;
    return n;
}

void Button::process(const float* const* in, int numIn,
                     float* const* out, int numOut,
                     int numSamples, const Transport&) {
    const bool pressNow = params.get("Press", 0.0) >= 0.5;
    const bool latch = params.get("Latch", 0.0) >= 0.5;
    if (!primed_) {
        primed_ = true;
        pressed_ = pressNow;
    }
    const bool rose = pressNow && !pressed_;
    pressed_ = pressNow;

    const bool wantOn = latch ? (rose ? !on_ : on_) : pressNow;
    if (wantOn != on_) {
        if (soundingNote_ >= 0) emit(false);
        if (wantOn) emit(true);
        on_ = wantOn;
    }
    ctl_.store(on_ ? 1.0f : 0.0f, std::memory_order_relaxed);

    if (numOut < 1) return;
    const float target = (float) params.get(on_ ? "On" : "Off", on_ ? 1.0 : 0.0);
    const double slewS = std::max(0.0, params.get("Slew", 5.0)) * 0.001;
    const float coef = slewS <= 0.0
        ? 1.0f
        : 1.0f - (float) std::exp(-1.0 / (slewS * sampleRate_));

    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    float* dst = out[0];
    for (int i = 0; i < numSamples; ++i) {
        smoothed_ += (target - smoothed_) * coef;
        dst[i] = (src ? src[i] : 0.0f) + smoothed_;
    }
    for (int c = 1; c < numOut; ++c)
        std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
}

}
