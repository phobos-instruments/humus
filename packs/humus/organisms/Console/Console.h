// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/Organism.h"
#include "hum/ParamRef.h"
#include "hum/dsp/LevelMeter.h"
#include "hum/dsp/SmoothedGain.h"

namespace hum {

class Console : public Organism, public LevelMeterSource, public WantsNullInlets {
public:
    Console(int numInputs, int width) : numInputs_(numInputs), width_(width) {
        for (int k = 0; k < numInputs; ++k) strips_.push_back(stripParams(k, width));
    }

    int numAudioInputs() const override { return numInputs_ * width_; }
    int numAudioOutputs() const override { return width_; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        meter_.prepare(sampleRate);
        outputS_.prepare(sampleRate);
        for (auto* s : {&driveS_, &xtalkS_, &stressS_}) s->prepare(sampleRate, kToneRampMs);
        reset();
    }

    int meterChannels() const override { return numInputs_; }
    float meterLevel(int strip) const override {
        if (strip < 0 || strip >= numInputs_) return 0.0f;
        float v = 0.0f;
        for (int side = 0; side < width_; ++side)
            v = std::max(v, meter_.level(strip * width_ + side));
        return v;
    }

    void reset() override {
        meter_.reset();
        xtLpL_ = xtLpR_ = 0.0;
        sagEnv_ = 0.0;
        dcXL_ = dcYL_ = dcXR_ = dcYR_ = 0.0;
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    static constexpr double kToneRampMs = 30.0;
    struct Strip { ParamRef mute, solo, gain, pan; };
    static Strip stripParams(int k, int width);

    int numInputs_;
    int width_;
    std::vector<Strip> strips_;
    LevelMeter meter_;
    SmoothedGain outputS_, driveS_, xtalkS_, stressS_;

    double xtLpL_ = 0.0, xtLpR_ = 0.0;
    double sagEnv_ = 0.0;
    double dcXL_ = 0.0, dcYL_ = 0.0, dcXR_ = 0.0, dcYR_ = 0.0;
};

}
