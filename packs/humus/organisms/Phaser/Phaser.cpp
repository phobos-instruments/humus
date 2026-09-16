// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Phaser/Phaser.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void Phaser::process(const float* const* in, int numIn, float* const* out, int numOut,
                     int numSamples, const Transport&) {
    const double fMin = std::max(20.0, params.get("FrequencyRange", 100.0));
    const double fMax = std::max(fMin + 1.0, params.getMax("FrequencyRange", 4000.0));
    const double rate = params.get("Rate", 0.5);
    const float fb = (float) std::clamp(params.get("Feedback", 0.0), 0.0, 0.97);
    const float depth = (float) std::clamp(params.get("Depth", 0.5), 0.0, 1.0);
    lfo_.setRate(rate, sampleRate_);
    const double logMin = std::log(fMin), logMax = std::log(fMax);

    for (int n = 0; n < numSamples; ++n) {
        const double lfoVal = Lfo::sineUp(lfo_.tick());
        const double fc = std::exp(logMin + lfoVal * (logMax - logMin));
        const double tn = std::tan(kPi * std::min(fc, sampleRate_ * 0.49) / sampleRate_);
        const double a = (tn - 1.0) / (tn + 1.0);

        for (int c = 0; c < numOut && c < 2; ++c) {
            const float dry = (c < numIn && in[c]) ? in[c][n] : 0.0f;
            double x = dry + fb * fbk_[(size_t) c];
            for (int s = 0; s < kStages; ++s) {
                double& z = ap_[(size_t) c][(size_t) s];
                const double y = a * x + z;
                z = x - a * y;
                x = y;
            }
            fbk_[(size_t) c] = x;
            out[c][n] = dry * (1.0f - depth) + (float) x * depth;
        }
    }
}

}
