// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/Organism.h"

#include "common/Stft.h"

namespace hum {

class SpectralFilter : public Organism, public LatencyReporting {
public:
    explicit SpectralFilter(int channels) : ch_(channels) {}

    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int latencySamples() const override {
        return chans_.empty() ? 0 : chans_.front().stft.latencySamples();
    }

private:
    struct Chan {
        Stft stft;
        std::vector<float> mag, env, dry;
        int dryPos = 0;
    };
    void onFrame(Chan& c, float* reim, int fftSize);

    int ch_;
    std::vector<Chan> chans_;
    std::vector<float> wet_, silence_;
    float tilt_ = 0.0f, contrast_ = 0.0f;
};

}
