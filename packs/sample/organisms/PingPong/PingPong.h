// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: MIT
#pragma once
#include "hum/Organism.h"
#include "hum/dsp/DelayLine.h"

namespace hum {

class PingPong : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override { left_.clear(); right_.clear(); }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    DelayLine left_, right_;
};

}
