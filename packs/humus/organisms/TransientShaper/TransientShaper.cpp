// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "TransientShaper/TransientShaper.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr double kFastAtkMs = 0.5,  kFastRelMs = 40.0;
constexpr double kSlowAtkMs = 20.0, kSlowRelMs = 250.0;
constexpr double kGainSmMs  = 3.0;
constexpr double kMaxDb     = 15.0;
constexpr double kFloor     = 1e-3;
}

void TransientShaper::process(const float* const* in, int numIn, float* const* out, int numOut,
                              int numSamples, const Transport&) {
    const double attackDb  = std::clamp(params.get("Attack", 0.0),  -1.0, 1.0) * kMaxDb;
    const double sustainDb = std::clamp(params.get("Sustain", 0.0), -1.0, 1.0) * kMaxDb;
    const float outGain = (float) params.get("OutputGain", 1.0);

    fast_.set(kFastAtkMs, kFastRelMs, sampleRate_);
    slow_.set(kSlowAtkMs, kSlowRelMs, sampleRate_);
    const double gsm = smoothCoeff(kGainSmMs, sampleRate_);

    for (int n = 0; n < numSamples; ++n) {
        const double x = linkedPeak(in, numIn, 0, ch_, n);
        const double envFast = fast_.process(x);
        const double envSlow = slow_.process(x);

        double target = 1.0;
        if (envSlow > kFloor) {
            const double norm = (envFast - envSlow) / envSlow;
            double a = norm > 0.0 ? norm : 0.0;
            double s = norm < 0.0 ? -norm : 0.0;
            a = a / (1.0 + a);
            s = s / (1.0 + s);
            target = dbToLin(attackDb * a + sustainDb * s);
        }
        gainSm_ = gsm * gainSm_ + (1.0 - gsm) * target;

        const float g = (float) gainSm_ * outGain;
        for (int c = 0; c < numOut; ++c)
            out[c][n] = ((c < numIn && in[c]) ? in[c][n] : 0.0f) * g;
    }
}

}
