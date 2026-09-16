// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"

namespace hum {

class Bus : public Organism {
public:
    Bus(int numInputs, int width) : numInputs_(numInputs), width_(width) {}

    int numAudioInputs() const override { return numInputs_ * width_; }
    int numAudioOutputs() const override { return width_; }

    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    int numInputs_;
    int width_;
};

}
