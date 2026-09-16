// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

class TimecodeTracker {
public:
    void prepare(double sampleRate, double nominalHz) {
        sr_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        setNominal(nominalHz);
        omegaSmooth_ = 0.0;
        env_ = 0.0;
        havePhase_ = false;
        const double tau = 0.010;
        omegaCoef_ = 1.0 - std::exp(-1.0 / (tau * sr_));
        envCoef_ = 1.0 - std::exp(-1.0 / (0.005 * sr_));
    }

    void setNominal(double nominalHz) {
        nominalStep_ = 2.0 * kPi * (nominalHz > 1.0 ? nominalHz : 1000.0)
                       / (sr_ > 0.0 ? sr_ : 48000.0);
    }

    void push(float l, float r) {
        env_ += ((double) l * l + (double) r * r - env_) * envCoef_;
        const double theta = std::atan2((double) r, (double) l);
        if (!havePhase_) { phase_ = theta; havePhase_ = true; return; }
        double d = theta - phase_;
        phase_ = theta;
        while (d > kPi) d -= 2.0 * kPi;
        while (d < -kPi) d += 2.0 * kPi;
        const double lim = nominalStep_ * 8.0;
        if (d > lim || d < -lim) return;
        omegaSmooth_ += (d - omegaSmooth_) * omegaCoef_;
    }

    double speed() const {
        return nominalStep_ > 0.0 ? omegaSmooth_ / nominalStep_ : 0.0;
    }

    bool present() const { return env_ > 2.5e-5; }

private:
    double sr_ = 48000.0;
    double nominalStep_ = 0.13;
    double phase_ = 0.0;
    bool havePhase_ = false;
    double omegaSmooth_ = 0.0, omegaCoef_ = 0.01;
    double env_ = 0.0, envCoef_ = 0.01;
};

}
