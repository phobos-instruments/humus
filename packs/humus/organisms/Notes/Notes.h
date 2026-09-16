// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"

namespace hum {

class Notes : public Organism {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}
};

}
