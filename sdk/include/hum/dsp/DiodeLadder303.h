#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

class DiodeLadder303 {
public:
    void prepare(double sampleRate) {
        sr_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        gHp_ = std::tan(3.14159265358979 * 150.0 / sr_);
        gHp_ = gHp_ / (1.0 + gHp_);
        smooth_ = 1.0 - std::exp(-1.0 / (0.0015 * sr_));
        reset();
    }

    void reset() {
        s1_ = s2_ = s3_ = s4_ = sHp_ = 0.0;
        fcSm_ = fcTarget_;
    }

    void setTarget(double cutoffHz, double res) {
        fcTarget_ = std::clamp(cutoffHz, 20.0, sr_ * 0.45);
        const double r = std::clamp(res, 0.0, 1.0);
        const double skew = std::pow(r, 1.35);
        k_ = 4.4 * skew;
        makeup_ = 1.0 + 1.1 * skew;
    }

    float process(float in, double fmOctaves = 0.0) {
        fcSm_ += (fcTarget_ - fcSm_) * smooth_;
        const double fc = fmOctaves == 0.0
            ? fcSm_
            : std::clamp(fcSm_ * std::exp2(std::clamp(fmOctaves, -3.0, 3.0)),
                         20.0, sr_ * 0.45);
        double g = std::tan(3.14159265358979 * fc / sr_);
        const double G = g / (1.0 + g);

        const double vHp = (s4_ - sHp_) * gHp_;
        const double hp = s4_ - (vHp + sHp_);
        sHp_ += 2.0 * vHp;

        double x = (double) in - k_ * hp;
        x = std::clamp(x, -1.41421356, 1.41421356);
        x = x - x * x * x * (1.0 / 6.0);

        auto stage = [&](double xin, double& s) {
            const double v = (xin - s) * G;
            const double y = v + s;
            s = y + v;
            return y;
        };
        const double y1 = stage(x, s1_);
        const double y2 = stage(y1, s2_);
        const double y3 = stage(y2, s3_);
        const double y4 = stage(y3, s4_);
        return (float) (y4 * makeup_);
    }

private:
    double sr_ = 44100.0;
    double gHp_ = 0.02, smooth_ = 0.02;
    double fcTarget_ = 900.0, fcSm_ = 900.0;
    double k_ = 0.0, makeup_ = 1.0;
    double s1_ = 0.0, s2_ = 0.0, s3_ = 0.0, s4_ = 0.0, sHp_ = 0.0;
};

}
