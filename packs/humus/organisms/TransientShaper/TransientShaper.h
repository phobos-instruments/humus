// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"
#include "hum/dsp/EnvelopeFollower.h"

namespace hum {

class TransientShaper : public Organism {
public:
    explicit TransientShaper(int channels) : ch_(channels) {}
    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { fast_.reset(); slow_.reset(); gainSm_ = 1.0; }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    int ch_;
    EnvelopeFollower fast_;
    EnvelopeFollower slow_;
    double gainSm_ = 1.0;
};

}
