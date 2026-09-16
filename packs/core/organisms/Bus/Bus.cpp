// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Bus/Bus.h"

#include <algorithm>

namespace hum {

void Bus::process(const float* const* in, int numIn,
                  float* const* out, int numOut,
                  int numSamples, const Transport&) {
    for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
    for (int k = 0; k < numInputs_; ++k)
        for (int c = 0; c < width_ && c < numOut; ++c) {
            int ch = k * width_ + c;
            if (ch >= numIn || !in[ch]) continue;
            for (int n = 0; n < numSamples; ++n) out[c][n] += in[ch][n];
        }
}

}
