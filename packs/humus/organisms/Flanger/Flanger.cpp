// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Flanger/Flanger.h"

#include <algorithm>
#include <cmath>

namespace hum {

void Flanger::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    int maxSamples = (int) (sampleRate / 20.0) + 4;
    lines_[0].prepare(maxSamples);
    lines_[1].prepare(maxSamples);
}

void Flanger::process(const float* const* in, int numIn,
                      float* const* out, int numOut,
                      int numSamples, const Transport&) {
    const float fMin = std::max(20.0f, (float) params.get("FrequencyRange", 100.0));
    const float fMax = std::max(fMin + 1.0f, (float) params.getMax("FrequencyRange", 1600.0));
    const float rate = (float) params.get("Rate", 0.5);
    const float fb = (float) params.get("Feedback", 0.0);
    const float mix = (float) params.get("WetDryMix", 0.5);
    lfo_.setRate(rate, sampleRate_);
    const float logMin = std::log(fMin), logMax = std::log(fMax);

    for (int n = 0; n < numSamples; ++n) {
        const float lfoVal = (float) Lfo::sineUp(lfo_.tick());
        float freq = std::exp(logMin + lfoVal * (logMax - logMin));
        float delay = (float) (sampleRate_ / freq);

        for (int c = 0; c < numOut && c < 2; ++c) {
            float x = (c < numIn && in[c]) ? in[c][n] : 0.0f;
            float d = lines_[c].read(delay);
            lines_[c].write(x + fb * d);
            out[c][n] = (1.0f - mix) * x + mix * (x + d);
        }
    }
}

}
