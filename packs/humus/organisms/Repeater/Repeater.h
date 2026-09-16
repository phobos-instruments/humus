// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <vector>

#include "hum/Organism.h"

namespace hum {

class Repeater : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override;
    void reset() override {
        engaged_ = false;
        w_ = 0.0;
        play_ = 0;
        lastLen_ = 0;
        fadeLeft_ = 0;
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    std::vector<float> buf_[2];
    long long ring_ = 0;
    long long pos_ = 0;
    long long regionEnd_ = 0;
    long long play_ = 0;
    bool engaged_ = false;
    double w_ = 0.0;
    long long lastLen_ = 0;
    long long fadeFrom_ = 0;
    int fadeLeft_ = 0;
    int xfN_ = 240;
};

}
