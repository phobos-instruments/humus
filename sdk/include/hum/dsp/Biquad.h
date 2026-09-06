#pragma once
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    void reset() { x1 = x2 = y1 = y2 = 0; }

    float process(float in) {
        const double x = in;
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return (float) y;
    }

    void normalise(double b0_, double b1_, double b2_, double a0_, double a1_, double a2_) {
        b0 = b0_ / a0_; b1 = b1_ / a0_; b2 = b2_ / a0_;
        a1 = a1_ / a0_; a2 = a2_ / a0_;
    }

    void setLowpass(double sr, double f0, double q) {
        f0 = clampFreq(sr, f0); q = q < 0.05 ? 0.05 : q;
        const double w = 2.0 * kPi * f0 / sr, c = std::cos(w), a = std::sin(w) / (2.0 * q);
        normalise((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a);
    }
    void setHighpass(double sr, double f0, double q) {
        f0 = clampFreq(sr, f0); q = q < 0.05 ? 0.05 : q;
        const double w = 2.0 * kPi * f0 / sr, c = std::cos(w), a = std::sin(w) / (2.0 * q);
        normalise((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a);
    }
    void setBandpass(double sr, double f0, double q) {
        f0 = clampFreq(sr, f0); q = q < 0.05 ? 0.05 : q;
        const double w = 2.0 * kPi * f0 / sr, c = std::cos(w), a = std::sin(w) / (2.0 * q);
        normalise(a, 0.0, -a, 1 + a, -2 * c, 1 - a);
    }
    void setPeaking(double sr, double f0, double q, double gainDb) {
        f0 = clampFreq(sr, f0); q = q < 0.05 ? 0.05 : q;
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * f0 / sr, c = std::cos(w), a = std::sin(w) / (2.0 * q);
        normalise(1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A);
    }
    void setLowShelf(double sr, double f0, double gainDb) { shelf(sr, f0, gainDb, true); }
    void setHighShelf(double sr, double f0, double gainDb) { shelf(sr, f0, gainDb, false); }
    void setAllpass(double sr, double f0, double q) {
        f0 = clampFreq(sr, f0); q = q < 0.05 ? 0.05 : q;
        const double w = 2.0 * kPi * f0 / sr, c = std::cos(w), a = std::sin(w) / (2.0 * q);
        normalise(1 - a, -2 * c, 1 + a, 1 + a, -2 * c, 1 - a);
    }
    void setAllpass1(double sr, double f0) {
        f0 = clampFreq(sr, f0);
        const double t = std::tan(kPi * f0 / sr);
        const double a = (t - 1.0) / (t + 1.0);
        normalise(a, 1.0, 0.0, 1.0, a, 0.0);
    }

private:
    static double clampFreq(double sr, double f0) {
        const double hi = sr * 0.49;
        return f0 < 10.0 ? 10.0 : (f0 > hi ? hi : f0);
    }
    void shelf(double sr, double f0, double gainDb, bool low) {
        f0 = clampFreq(sr, f0);
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * f0 / sr, c = std::cos(w);
        const double alpha = std::sin(w) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / 0.9 - 1.0) + 2.0);
        const double tsa = 2.0 * std::sqrt(A) * alpha;
        const double s = low ? 1.0 : -1.0;
        normalise(A * ((A + 1) - s * (A - 1) * c + tsa),
                  2 * A * s * ((A - 1) - s * (A + 1) * c),
                  A * ((A + 1) - s * (A - 1) * c - tsa),
                  (A + 1) + s * (A - 1) * c + tsa,
                  -2 * s * ((A - 1) + s * (A + 1) * c),
                  (A + 1) + s * (A - 1) * c - tsa);
    }
};

}
