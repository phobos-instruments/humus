// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "ParaEQ/ParaEQ.h"

#include <algorithm>

namespace hum {

void ParaEQ::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
}

void ParaEQ::reset() {
    for (auto& ch : bands_)
        for (auto& b : ch) b.reset();
}

void ParaEQ::process(const float* const* in, int numIn, float* const* out, int numOut,
                     int numSamples, const Transport&) {
    const double lsF = params.get("LSCutoffFrequency", 120.0), lsG = params.get("LSGain", 0.0);
    const double bp1F = params.get("BP1CenterFrequency", 1000.0);
    const double bp1B = std::max(1.0, params.get("BP1Bandwidth", 200.0)), bp1G = params.get("BP1Gain", 0.0);
    const double bp2F = params.get("BP2CenterFrequency", 3000.0);
    const double bp2B = std::max(1.0, params.get("BP2Bandwidth", 200.0)), bp2G = params.get("BP2Gain", 0.0);
    const double hsF = params.get("HSCutoffFrequency", 8000.0), hsG = params.get("HSGain", 0.0);

    for (auto& ch : bands_) {
        ch[0].setLowShelf(sampleRate_, lsF, lsG);
        ch[1].setPeaking(sampleRate_, bp1F, bp1F / bp1B, bp1G);
        ch[2].setPeaking(sampleRate_, bp2F, bp2F / bp2B, bp2G);
        ch[3].setHighShelf(sampleRate_, hsF, hsG);
    }

    for (int c = 0; c < numOut; ++c) {
        auto& b = bands_[(size_t) std::min(c, ch_ - 1)];
        for (int n = 0; n < numSamples; ++n) {
            float x = (c < numIn && in[c]) ? in[c][n] : 0.0f;
            x = b[0].process(x); x = b[1].process(x);
            x = b[2].process(x); x = b[3].process(x);
            out[c][n] = x;
        }
    }
}

}
