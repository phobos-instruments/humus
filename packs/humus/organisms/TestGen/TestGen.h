// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"

namespace hum {

class TestGen : public Organism {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override { phase_ = 0.0; for (auto& v : pink_) v = 0.0f; brown_ = 0.0f; }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    double phase_ = 0.0;
    uint32_t rngState_ = 0x12345678u;
    float pink_[3] = {};
    float brown_ = 0.0f;
    float nextNoise();
    float nextPink();
    float nextBrown();
};

}
