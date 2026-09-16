// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"
#include "hum/dsp/SmoothedGain.h"

namespace hum {

class Send : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 4; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        level_.prepare(sampleRate);
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    SmoothedGain level_;
};

}
