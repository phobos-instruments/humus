// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"

#include "ShimmerEngine.h"

namespace hum {

class Bloom : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override {
        sampleRate_ = sampleRate;
        engine_.prepare(sampleRate, maxBlock);
    }
    void reset() override { engine_.reset(); }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    sv::ShimmerEngine engine_;
};

}
