#pragma once
#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

class DiodeLadder303 {
public:
    static constexpr int kOversample = 4;
    static constexpr double kFeedbackHighpassHz = 150.0;

    void prepare(double sampleRate) {
        sr_ = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
        inner_ = sr_ * kOversample;
        setFeedbackHighpass(kFeedbackHighpassHz);
        const double w = std::tan(kPi * std::min(0.45 * sr_, 20000.0) / inner_);
        const double q = kSqrtHalf;
        const double a0 = 1.0 + w / q + w * w;
        decB0_ = w * w / a0;
        decB1_ = 2.0 * decB0_;
        decA1_ = 2.0 * (w * w - 1.0) / a0;
        decA2_ = (1.0 - w / q + w * w) / a0;
        smooth_ = 1.0 - std::exp(-1.0 / (0.0015 * sr_));
        reset();
    }

    void reset() {
        y1_ = y2_ = y3_ = y4_ = 0.0;
        hpX1_ = hpY1_ = 0.0;
        decX1_ = decX2_ = decY1_ = decY2_ = 0.0;
        fcSm_ = fcTarget_;
    }

    void setFeedbackHighpass(double hz) {
        const double x = std::exp(-kTwoPi * std::clamp(hz, 20.0, 2000.0) / inner_);
        hpB0_ = 0.5 * (1.0 + x);
        hpA1_ = x;
    }

    void setTarget(double cutoffHz, double res) {
        fcTarget_ = std::clamp(cutoffHz, 20.0, sr_ * 0.45);
        const double r = std::clamp(res, 0.0, 1.0);
        r_ = (1.0 - std::exp(-3.0 * r)) / (1.0 - std::exp(-3.0));
    }

    float process(float in, double fmOctaves = 0.0) {
        fcSm_ += (fcTarget_ - fcSm_) * smooth_;
        const double fc = fmOctaves == 0.0
            ? fcSm_
            : std::clamp(fcSm_ * std::exp2(std::clamp(fmOctaves, -3.0, 3.0)),
                         20.0, sr_ * 0.45);
        const double fx = fc * kSqrtHalf / inner_;
        const double b0 = (0.00045522346 + 6.1922189 * fx)
                          / (1.0 + 12.358354 * fx + 4.4156345 * fx * fx);
        double k = fx * (fx * (fx * (fx * (fx * (fx + 7198.6997) - 5837.7917) - 476.47308)
                              + 614.95611) + 213.87126) + 16.998792;
        double g = k / 17.0;
        g = ((g - 1.0) * r_ + 1.0) * (1.0 + r_);
        k *= r_;

        double out = 0.0;
        for (int n = 0; n < kOversample; ++n) {
            const double fb = k * y4_;
            const double hp = hpB0_ * (fb - hpX1_) + hpA1_ * hpY1_;
            hpX1_ = fb;
            hpY1_ = hp;
            double y0 = std::clamp((double) in - hp, -1.41421356, 1.41421356);
            y0 = y0 - y0 * y0 * y0 * (1.0 / 6.0);
            y1_ += 2.0 * b0 * (y0 - y1_ + y2_);
            y2_ += b0 * (y1_ - 2.0 * y2_ + y3_);
            y3_ += b0 * (y2_ - 2.0 * y3_ + y4_);
            y4_ += b0 * (y3_ - 2.0 * y4_);
            const double x = 2.0 * g * y4_;
            out = decB0_ * x + decB1_ * decX1_ + decB0_ * decX2_ - decA1_ * decY1_ - decA2_ * decY2_;
            decX2_ = decX1_;
            decX1_ = x;
            decY2_ = decY1_;
            decY1_ = out;
        }
        return (float) out;
    }

private:
    double sr_ = kDefaultSampleRate, inner_ = kDefaultSampleRate * kOversample;
    double hpB0_ = 0.5, hpA1_ = 0.0, smooth_ = 0.02;
    double decB0_ = 1.0, decB1_ = 0.0, decA1_ = 0.0, decA2_ = 0.0;
    double fcTarget_ = 900.0, fcSm_ = 900.0, r_ = 0.0;
    double y1_ = 0.0, y2_ = 0.0, y3_ = 0.0, y4_ = 0.0;
    double hpX1_ = 0.0, hpY1_ = 0.0;
    double decX1_ = 0.0, decX2_ = 0.0, decY1_ = 0.0, decY2_ = 0.0;
};

}
