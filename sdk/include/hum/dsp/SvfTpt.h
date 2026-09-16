// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

struct SvfTpt {
    double ic1 = 0.0, ic2 = 0.0;

    struct Out {
        double hp, bp, lp;
    };

    void reset() { ic1 = ic2 = 0.0; }

    Out process(double x, double g, double k) {
        const double v1 = (ic1 + g * (x - ic2)) / (1.0 + g * (g + k));
        const double v2 = ic2 + g * v1;
        const double hp = x - k * v1 - v2;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;
        return {hp, v1, v2};
    }

    static double gFor(double fc, double sampleRate) {
        return std::tan(kPi * fc / sampleRate);
    }
};

}
