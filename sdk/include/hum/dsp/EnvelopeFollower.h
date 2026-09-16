// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "hum/dsp/DspMath.h"

namespace hum {

struct EnvelopeFollower {
    void set(double attackMs, double releaseMs, double sampleRate) {
        atk_ = smoothCoeff(attackMs, sampleRate);
        rel_ = smoothCoeff(releaseMs, sampleRate);
    }

    void reset(double v = 0.0) { env_ = v; }

    double process(double x) {
        const double c = x > env_ ? atk_ : rel_;
        env_ = c * env_ + (1.0 - c) * x;
        return env_;
    }

    double value() const { return env_; }

private:
    double atk_ = 0.0, rel_ = 0.0;
    double env_ = 0.0;
};

}
