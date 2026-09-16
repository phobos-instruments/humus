// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/Organism.h"
#include "hum/dsp/LevelMeter.h"
#include "hum/dsp/SmoothedGain.h"

namespace hum {

class Mixer : public Organism, public LevelMeterSource, public WantsNullInlets {
public:
    Mixer(int numInputs, int width, bool pan = false)
        : numInputs_(numInputs), width_(width), pan_(pan) {
        for (int k = 0; k < numInputs_; ++k) {
            const std::string sfx = inputSuffix(k);
            keys_.push_back({"Gain_" + sfx, "Mute_" + sfx, "Solo_" + sfx, "Pan_" + sfx});
        }
        gainL_.resize((size_t) numInputs_);
        gainR_.resize((size_t) numInputs_);
    }

    int numAudioInputs() const override { return numInputs_ * width_; }
    int numAudioOutputs() const override { return pan_ ? 2 : width_; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        meter_.prepare(sampleRate);
        for (auto& g : gainL_) g.prepare(sampleRate);
        for (auto& g : gainR_) g.prepare(sampleRate);
    }
    void reset() override { meter_.reset(); }

    int meterChannels() const override { return numInputs_; }
    float meterLevel(int strip) const override {
        if (strip < 0 || strip >= numInputs_) return 0.0f;
        float v = 0.0f;
        for (int side = 0; side < width_; ++side)
            v = std::max(v, meter_.level(strip * width_ + side));
        return v;
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    struct Keys { std::string gain, mute, solo, pan; };

    int numInputs_;
    int width_;
    bool pan_;
    LevelMeter meter_;
    std::vector<Keys> keys_;
    std::vector<SmoothedGain> gainL_;
    std::vector<SmoothedGain> gainR_;
    std::string inputSuffix(int k) const;
};

}
