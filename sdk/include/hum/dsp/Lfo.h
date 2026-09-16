// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

struct Lfo {
    void setRate(double hz, double sampleRate) {
        inc_ = sampleRate > 0.0 ? hz / sampleRate : 0.0;
    }
    void setPeriodSamples(double samples) { inc_ = samples > 0.0 ? 1.0 / samples : 0.0; }

    void reset(double phase = 0.0) { phase_ = phase; }

    double tick() {
        const double p = phase_;
        phase_ += inc_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return p;
    }

    double phase() const { return phase_; }

    static double wrap(double p) { return p - std::floor(p); }

    static double sine(double p) { return std::sin(2.0 * kPi * p); }
    static double sineUp(double p) { return 0.5 * (1.0 + sine(p)); }
    static double triangle(double p) { return 1.0 - 4.0 * std::abs(p - 0.5); }
    static double triangleUp(double p) { return 1.0 - std::abs(2.0 * p - 1.0); }
    static double square(double p) { return p < 0.5 ? 1.0 : -1.0; }
    static double sawUp(double p) { return 2.0 * p - 1.0; }
    static double sawDown(double p) { return 1.0 - 2.0 * p; }

private:
    double phase_ = 0.0, inc_ = 0.0;
};

}
