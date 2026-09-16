// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

class SmoothedGain {
public:
    static constexpr double kDefaultRampMs = 5.0;

    void prepare(double sampleRate, double rampMs = kDefaultRampMs) {
        rampLen_ = std::max(1, (int) std::lround(rampMs * 0.001 * sampleRate));
        primed_ = false;
        remaining_ = 0;
    }

    void snapTo(float v) {
        cur_ = target_ = v;
        remaining_ = 0;
        primed_ = true;
    }

    void setTarget(float t) {
        if (!primed_) { snapTo(t); return; }
        if (t == target_) return;
        target_ = t;
        remaining_ = rampLen_;
        step_ = (t - cur_) / (float) rampLen_;
    }

    bool ramping() const { return remaining_ > 0; }
    bool silent() const { return remaining_ == 0 && cur_ == 0.0f; }
    float current() const { return cur_; }
    float target() const { return target_; }
    int rampLength() const { return rampLen_; }

    float next() {
        if (remaining_ > 0) {
            cur_ += step_;
            if (--remaining_ == 0) cur_ = target_;
        }
        return cur_;
    }

    void apply(const float* in, float* out, int n) {
        if (!ramping()) {
            const float g = cur_;
            for (int i = 0; i < n; ++i) out[i] = in[i] * g;
            return;
        }
        for (int i = 0; i < n; ++i) out[i] = in[i] * next();
    }

    void applyAdd(const float* in, float* out, int n) {
        if (!ramping()) {
            const float g = cur_;
            if (g == 0.0f) return;
            for (int i = 0; i < n; ++i) out[i] += in[i] * g;
            return;
        }
        for (int i = 0; i < n; ++i) out[i] += in[i] * next();
    }

    void skip(int n) {
        for (int i = 0; i < n && remaining_ > 0; ++i) next();
    }

private:
    float cur_ = 0.0f;
    float target_ = 0.0f;
    float step_ = 0.0f;
    int rampLen_ = 1;
    int remaining_ = 0;
    bool primed_ = false;
};

}
