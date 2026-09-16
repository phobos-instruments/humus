// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>

#include "hum/dsp/Lfo.h"
#include "hum/Organism.h"

namespace hum {

class Phaser : public Organism {
public:
    static constexpr int kStages = 6;
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { ap_ = {}; fbk_ = {}; lfo_.reset(); }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    std::array<std::array<double, kStages>, 2> ap_{};
    std::array<double, 2> fbk_{};
    Lfo lfo_;
};

}
