// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {

class PitchShifter {
public:
    enum class Blend { Power, Level };

    static int wholeCycleWindow(double periodSamples, double sampleRate, double seconds = 0.04) {
        const double period = std::max(1.0, periodSamples);
        const double cycles = std::max(1.0, std::round(seconds * sampleRate / (2.0 * period)));
        return (int) std::lround(2.0 * cycles * period);
    }

    void prepare(double sampleRate, double windowMs = 60.0) {
        window_ = std::max(64, (int) (sampleRate * windowMs / 1000.0));
        buf_.assign((size_t) (window_ * 2 + 4), 0.0f);
        fadeLength_ = std::max(1, (int) (sampleRate * kWindowFadeSeconds));
        reset();
    }
    void reset() {
        std::fill(buf_.begin(), buf_.end(), 0.0f);
        writePos_ = 0;
        phase_ = 0.0f;
        fadeLeft_ = 0;
        pendingWindow_ = 0;
    }
    void setRatio(double r) { ratio_ = (float) std::clamp(r, 0.25, 4.0); }
    void setBlend(Blend b) { blend_ = b; }

    void setWindow(int samples) { window_ = clampedWindow(samples); }

    void glideToWindow(int samples) {
        const int w = clampedWindow(samples);
        if (fadeLeft_ > 0) { pendingWindow_ = w; return; }
        if (w == window_) return;
        oldWindow_ = window_;
        oldPhase_ = phase_;
        window_ = w;
        phase_ = oldPhase_ * (float) w / (float) oldWindow_;
        fadeLeft_ = fadeLength_;
    }
    int window() const { return pendingWindow_ > 0 ? pendingWindow_ : window_; }

    float process(float x) {
        const int size = (int) buf_.size();
        buf_[(size_t) writePos_] = x;
        float out = taps(window_, phase_);
        advance(phase_, window_);
        if (fadeLeft_ > 0) {
            const float older = taps(oldWindow_, oldPhase_);
            advance(oldPhase_, oldWindow_);
            const float t = (float) fadeLeft_ / (float) fadeLength_;
            out = out * (1.0f - t) + older * t;
            if (--fadeLeft_ == 0 && pendingWindow_ > 0) {
                const int next = pendingWindow_;
                pendingWindow_ = 0;
                glideToWindow(next);
            }
        }
        writePos_ = (writePos_ + 1) % size;
        return out;
    }

private:
    static constexpr double kWindowFadeSeconds = 0.02;

    int clampedWindow(int samples) const {
        const int maxWindow = ((int) buf_.size() - 4) / 2;
        return std::clamp(samples, 64, maxWindow);
    }

    float taps(int window, float phase) const {
        const float half = window * 0.5f;
        const float d1 = phase;
        float d2 = phase + half;
        if (d2 >= window) d2 -= window;
        const float pi = kPiF;
        float g1 = std::sin(pi * d1 / window);
        float g2 = std::sin(pi * d2 / window);
        if (blend_ == Blend::Level) {
            g1 *= g1;
            g2 *= g2;
        }
        return g1 * read(writePos_ - d1) + g2 * read(writePos_ - d2);
    }

    void advance(float& phase, int window) const {
        phase += (1.0f - ratio_);
        while (phase >= window) phase -= window;
        while (phase < 0.0f) phase += window;
    }

    float read(float pos) const {
        const int size = (int) buf_.size();
        while (pos < 0.0f) pos += size;
        while (pos >= size) pos -= size;
        const int i0 = (int) pos;
        const int i1 = (i0 + 1) % size;
        const float f = pos - i0;
        return buf_[(size_t) i0] * (1.0f - f) + buf_[(size_t) i1] * f;
    }

    std::vector<float> buf_;
    int window_ = 2646;
    int writePos_ = 0;
    float phase_ = 0.0f;
    float ratio_ = 1.0f;
    Blend blend_ = Blend::Power;
    int oldWindow_ = 2646;
    float oldPhase_ = 0.0f;
    int fadeLength_ = 960;
    int fadeLeft_ = 0;
    int pendingWindow_ = 0;
};

}
