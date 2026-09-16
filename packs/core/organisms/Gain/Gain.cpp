// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Gain/Gain.h"

#include <algorithm>

namespace hum {

void Gain::process(const float* const* in, int numIn,
                   float* const* out, int numOut,
                   int numSamples, const Transport&) {
    const bool muted = params.get("Mute", 0.0) >= 0.5;
    gain_.setTarget(muted ? 0.0f : (float) params.get("Gain", 1.0));
    if (gain_.ramping()) {
        for (int n = 0; n < numSamples; ++n) {
            const float g = gain_.next();
            for (int c = 0; c < numOut; ++c)
                out[c][n] = c < numIn && in[c] ? in[c][n] * g : 0.0f;
        }
    } else {
        const float g = gain_.current();
        for (int c = 0; c < numOut; ++c) {
            if (c < numIn && in[c])
                for (int n = 0; n < numSamples; ++n) out[c][n] = in[c][n] * g;
            else
                std::fill(out[c], out[c] + numSamples, 0.0f);
        }
    }
    meter_.measure(out, numOut, numSamples);
}

}
