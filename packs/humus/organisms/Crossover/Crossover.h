// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>

#include "hum/Organism.h"
#include "hum/ParamRef.h"
#include "hum/dsp/Biquad.h"

namespace hum {

class Crossover : public Organism {
public:
    explicit Crossover(int bands);
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return bands_ * 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    static constexpr int kMaxBands = 5;

private:
    struct SlopeSpec { int stages; double q[4]; int comps; double cq[2]; bool invertHi; };
    static SlopeSpec specFor(int slope);
    void design(const double* f, const SlopeSpec& sp);

    struct Split { Biquad lp[4][2], hp[4][2]; };

    int bands_;
    std::array<double, kMaxBands - 1> def_ {};
    std::array<ParamRef, kMaxBands - 1> freq_ = numberedParams<kMaxBands - 1>("Freq");
    std::array<Split, kMaxBands - 1> splits_;
    Biquad ap_[kMaxBands - 1][kMaxBands - 1][2][2];
    std::array<double, kMaxBands - 1> lastF_ {};
    int lastSlope_ = -1;
};

}
