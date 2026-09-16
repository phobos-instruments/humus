// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

class Coupling303 {
public:
    static constexpr double kInputHighpassHz = 44.486;
    static constexpr double kOutputHighpassHz = 24.167;
    static constexpr double kAllpassHz = 14.008;
    static constexpr double kNotchHz = 7.5164;
    static constexpr double kNotchOctaves = 4.7;

    void prepare(double sampleRate) {
        const double sr = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
        hpIn_.set(kInputHighpassHz, sr);
        hpOut_.set(kOutputHighpassHz, sr);
        const double t = std::tan(kPi * kAllpassHz / sr);
        apX_ = (t - 1.0) / (t + 1.0);
        const double w = kTwoPi * kNotchHz / sr;
        const double s = std::sin(w), c = std::cos(w);
        const double alpha = s * std::sinh(0.5 * std::log(2.0) * kNotchOctaves * w / s);
        const double scale = 1.0 / (1.0 + alpha);
        nB0_ = scale;
        nB1_ = -2.0 * c * scale;
        nA1_ = -2.0 * c * scale;
        nA2_ = (1.0 - alpha) * scale;
        reset();
    }

    void reset() {
        hpIn_.reset();
        hpOut_.reset();
        apX1_ = apY1_ = 0.0;
        nX1_ = nX2_ = nY1_ = nY2_ = 0.0;
    }

    float pre(float in) { return (float) hpIn_.tick(in); }

    float post(float in) {
        const double ap = apX_ * (double) in + apX1_ - apX_ * apY1_;
        apX1_ = in;
        apY1_ = ap;
        const double hp = hpOut_.tick(ap);
        const double y = nB0_ * hp + nB1_ * nX1_ + nB0_ * nX2_ - nA1_ * nY1_ - nA2_ * nY2_;
        nX2_ = nX1_;
        nX1_ = hp;
        nY2_ = nY1_;
        nY1_ = y;
        return (float) y;
    }

private:
    struct OnePoleHighpass {
        double b0 = 0.5, a1 = 0.0, x1 = 0.0, y1 = 0.0;
        void set(double hz, double sr) {
            const double x = std::exp(-kTwoPi * hz / sr);
            b0 = 0.5 * (1.0 + x);
            a1 = x;
        }
        void reset() { x1 = y1 = 0.0; }
        double tick(double in) {
            y1 = b0 * (in - x1) + a1 * y1;
            x1 = in;
            return y1;
        }
    };

    OnePoleHighpass hpIn_, hpOut_;
    double apX_ = 0.0, apX1_ = 0.0, apY1_ = 0.0;
    double nB0_ = 1.0, nB1_ = 0.0, nA1_ = 0.0, nA2_ = 0.0;
    double nX1_ = 0.0, nX2_ = 0.0, nY1_ = 0.0, nY2_ = 0.0;
};

}
