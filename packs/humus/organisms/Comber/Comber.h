// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>

#include "hum/dsp/DelayLine.h"
#include "hum/Organism.h"
#include "hum/ParamRef.h"

namespace hum {

class Comber : public Organism {
public:
    static constexpr int kMaxCombs = 8;
    static constexpr int kMaxChannels = 2;
    static constexpr double kLowestHz = 16.0;
    static constexpr double kGlideSeconds = 0.02;

    explicit Comber(int channels) : channels_(channels) {}

    int numAudioInputs() const override { return channels_; }
    int numAudioOutputs() const override { return channels_; }
    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    struct Tooth {
        std::array<DelayLine, kMaxChannels> lines;
        float delay = 0.0f, level = 0.0f;
        float delayGoal = 0.0f, levelGoal = 0.0f, feedback = 0.0f;
        bool settled = false;
    };

    int activeCombs() const;
    void aim(int count);

    std::array<Tooth, kMaxCombs> teeth_;
    std::array<ParamRef, kMaxCombs> freqRef_ = numberedParams<kMaxCombs>("Frequency_");
    std::array<ParamRef, kMaxCombs> decayRef_ = numberedParams<kMaxCombs>("DecayTime_");
    std::array<ParamRef, kMaxCombs> gainRef_ = numberedParams<kMaxCombs>("Gain_");
    float glide_ = 1.0f;
    int channels_ = kMaxChannels;
};

}
