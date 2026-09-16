// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Slider/Slider.h"

#include <algorithm>
#include <cmath>

namespace hum {

namespace {

float mapped(float value, float lo, float hi, bool log) {
    if (log && lo > 0.0f && hi > 0.0f) return lo * std::pow(hi / lo, value);
    return lo + value * (hi - lo);
}

float targetOf(const ParameterSet& params) {
    return mapped((float) std::clamp(params.get("Value", 0.0), 0.0, 1.0),
                  (float) params.get("Min", 0.0), (float) params.get("Max", 1.0),
                  params.get("Log", 0.0) >= 0.5);
}

}

void Slider::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
    smoothed_ = targetOf(params);
    settled_ = true;
    ctl_.store(smoothed_, std::memory_order_relaxed);
}

void Slider::process(const float* const* in, int numIn,
                     float* const* out, int numOut,
                     int numSamples, const Transport&) {
    (void) in; (void) numIn; (void) out; (void) numOut;

    const float lo = (float) params.get("Min", 0.0);
    const float hi = (float) params.get("Max", 1.0);
    lo_.store(std::min(lo, hi), std::memory_order_relaxed);
    hi_.store(std::max(lo, hi), std::memory_order_relaxed);

    const float target = targetOf(params);
    const double slewSeconds = std::max(0.0, params.get("Slew", 5.0)) * 0.001;
    if (!settled_ || slewSeconds <= 0.0 || sampleRate_ <= 0.0) {
        smoothed_ = target;
        settled_ = true;
    } else {
        const double block = (double) numSamples / sampleRate_;
        const double reach = 1.0 - std::exp(-block / slewSeconds);
        smoothed_ += (float) ((target - smoothed_) * reach);
        if (std::abs(target - smoothed_) < 1.0e-6f) smoothed_ = target;
    }

    ctl_.store(smoothed_, std::memory_order_relaxed);
}

}
