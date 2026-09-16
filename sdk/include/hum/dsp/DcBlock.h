// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "hum/dsp/DspMath.h"

namespace hum {

struct DcBlock {
    float R = 0.99929f;
    float x1 = 0.0f, y1 = 0.0f;

    void prepare(double sampleRate, double cornerHz = 5.0) {
        const double sr = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
        R = (float) (1.0 - kTwoPi * cornerHz / sr);
        x1 = y1 = 0.0f;
    }

    void process(const float* in, float* out, int n) {
        float lx = x1, ly = y1;
        for (int i = 0; i < n; ++i) {
            const float x = in[i];
            ly = x - lx + R * ly;
            lx = x;
            out[i] = ly;
        }
        x1 = lx;
        y1 = ly;
    }
};

}
