// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"
#include "hum/dsp/Biquad.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Phono : public Organism {
public:
    static constexpr int kNumB = 8;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    struct RiaaSection {
        double z[kNumB - 1] = {};
        void reset() { for (double& v : z) v = 0.0; }
        double process(const double* b, const double* a, double x) {
            const double y = b[0] * x + z[0];
            z[0] = b[1] * x - a[0] * y + z[1];
            z[1] = b[2] * x - a[1] * y + z[2];
            for (int i = 2; i < kNumB - 2; ++i) z[i] = b[i + 1] * x + z[i + 1];
            z[kNumB - 2] = b[kNumB - 1] * x;
            return y;
        }
    };
    struct OnePoleHp {
        double b0 = 1, b1 = -1, a1 = 0, x1 = 0, y1 = 0;
        void set(double sr, double f);
        void reset() { x1 = y1 = 0; }
        double process(double x) {
            const double y = b0 * x + b1 * x1 - a1 * y1;
            x1 = x; y1 = y;
            return y;
        }
    };

    void resetFilters();

    double sr_ = kDefaultSampleRate;
    double riaaB_[kNumB] = {}, riaaA_[2] = {};
    RiaaSection eq_[2];
    OnePoleHp iecHp_[2];
    OnePoleHp rumble1_[2];
    Biquad rumble2_[2];
    double gainSm_ = -1.0;
};

}
