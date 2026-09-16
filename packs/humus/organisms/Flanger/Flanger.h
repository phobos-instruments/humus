// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/dsp/DelayLine.h"
#include "hum/dsp/Lfo.h"
#include "hum/Organism.h"

namespace hum {

class Flanger : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override { lines_[0].clear(); lines_[1].clear(); lfo_.reset(); }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    DelayLine lines_[2];
    Lfo lfo_;
};

}
