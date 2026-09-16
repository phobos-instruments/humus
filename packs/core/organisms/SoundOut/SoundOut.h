// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/Organism.h"
#include "hum/dsp/DcBlock.h"
#include "hum/dsp/LevelMeter.h"

namespace hum {

class SoundOut : public Organism, public MasterTap, public LevelMeterSource {
public:
    int numAudioInputs() const override { return channels_; }
    int numAudioOutputs() const override { return 0; }
    void configureChannels(int inlets, int) override {
        channels_ = std::max(2, inlets);
    }
    int firstChannel() const override { return (int) params.get("Channel", 1.0) - 1; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override { meter_.reset(); gainSm_ = (float) params.get("Gain", 1.0); }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int channels() const override { return channels_; }
    int lastBlockLength() const override { return blockLen_; }
    const float* channelData(int c) const override { return block_[(size_t) c].data(); }

    int meterChannels() const override { return channels_; }
    float meterLevel(int ch) const override { return meter_.level(ch); }

private:
    int channels_ = 2;
    int blockLen_ = 0;
    float gainSm_ = 1.0f;
    std::vector<std::vector<float>> block_;
    std::vector<DcBlock> dc_;
    LevelMeter meter_;
};

}
