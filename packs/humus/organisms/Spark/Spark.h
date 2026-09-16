// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>

#include "hum/Organism.h"
#include "hum/dsp/DelayLine.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Spark : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    static constexpr int kLines = 6;

private:
    struct Line {
        DelayLine delay;
        double phase = 0.0;
        float damp = 0.0f;
    };

    double sampleRate_ = kDefaultSampleRate;
    Line lines_[2][kLines];
    float offset_[kLines] = {};
};

}
