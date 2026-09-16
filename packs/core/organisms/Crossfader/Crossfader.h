// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"

namespace hum {

class Crossfader : public Organism {
public:
    int numAudioInputs() const override { return 4; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override { seeded_ = false; }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    float position() const { return pos_; }

private:
    float pos_ = 0.5f;
    bool seeded_ = false;
};

}
