// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: MIT
#include "Tremolo.h"

#include <algorithm>
#include <cmath>

namespace hum {

void Tremolo::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    lfo_.reset();
}

void Tremolo::process(const float* const* in, int numIn,
                      float* const* out, int numOut,
                      int numSamples, const Transport& transport) {
    const double rate  = std::max(0.05, params.get("Rate", 4.0));
    const double depth = std::clamp(params.get("Depth", 0.8), 0.0, 1.0);
    const int    wave  = (int) params.get("Waveform", 0.0);
    const bool   sync  = params.get("Sync", 0.0) >= 0.5;

    const double periodSamples = sync ? std::max(1.0, transport.samplesPerBeat() * 0.5)
                                      : sampleRate_ / rate;
    lfo_.setPeriodSamples(periodSamples);

    for (int n = 0; n < numSamples; ++n) {
        const double p = lfo_.tick();
        double lfo;
        if (wave == 1)      lfo = Lfo::triangle(p);
        else if (wave == 2) lfo = Lfo::square(p);
        else                lfo = Lfo::sine(p);

        const float gain = (float) (1.0 - depth * (0.5 + 0.5 * lfo));

        for (int c = 0; c < numOut; ++c) {
            const float x = (c < numIn && in && in[c]) ? in[c][n] : 0.0f;
            out[c][n] = x * gain;
        }
    }
}

}
