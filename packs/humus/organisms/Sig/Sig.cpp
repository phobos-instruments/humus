// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Sig/Sig.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace hum {

void Sig::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    smoothed_ = (float) params.get("Value", 0.0);
}

void Sig::process(const float* const*, int, float* const* out, int numOut,
                  int numSamples, const Transport&) {
    if (numOut < 1) return;
    const float target = (float) params.get("Value", 0.0);
    const double slewS = std::max(0.0, params.get("Slew", 5.0)) * 0.001;
    const float coef = slewS <= 0.0 ? 1.0f : 1.0f - (float) std::exp(-1.0 / (slewS * sampleRate_));
    float* dst = out[0];
    for (int i = 0; i < numSamples; ++i) {
        smoothed_ += (target - smoothed_) * coef;
        dst[i] = smoothed_;
    }
    for (int c = 1; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
}

}
