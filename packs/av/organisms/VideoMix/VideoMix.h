// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/caps/Video.h"
#include "hum/Organism.h"

namespace hum {

inline constexpr int kVideoMixMostChannels = 8;

class VideoMix : public Organism, public VideoNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double, int) override {}
    void reset() override {}
    void process(const float* const*, int, float* const*, int, int,
                 const Transport&) override {}

    int numVideoInputs() const override { return 2; }
    int numVideoOutputs() const override { return 1; }
};

class VideoChannelMix : public Organism, public VideoNode {
public:
    explicit VideoChannelMix(int channels) : channels_(channels) {}
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double, int) override {}
    void reset() override {}
    void process(const float* const*, int, float* const*, int, int,
                 const Transport&) override {}

    int numVideoInputs() const override { return channels_; }
    int numVideoOutputs() const override { return 1; }

private:
    const int channels_;
};

}
