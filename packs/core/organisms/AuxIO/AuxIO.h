// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/Organism.h"

namespace hum {

class AuxIn : public Organism, public HardwareIn {
public:
    explicit AuxIn(int hardwareChannel = 2) : default_(hardwareChannel) {}
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 1; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    int channel() const override {
        return (int) params.get("Channel", default_ + 1.0) - 1;
    }
    void process(const float* const*, int, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override {
        const float* src = transport.liveInput(channel());
        for (int c = 0; c < numOut; ++c) {
            if (c == 0 && src) std::copy(src, src + numSamples, out[c]);
            else std::fill(out[c], out[c] + numSamples, 0.0f);
        }
    }

private:
    int default_ = 2;
};

class AuxOut : public Organism, public HardwareOut {
public:
    explicit AuxOut(int hardwareChannel = 2) : default_(hardwareChannel) {}
    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override {
        sampleRate_ = sampleRate;
        buffer_.assign((size_t) std::max(1, maxBlock), 0.0f);
    }
    void process(const float* const* in, int numIn, float* const*, int,
                 int numSamples, const Transport&) override {
        blockLen_ = std::min(numSamples, (int) buffer_.size());
        if (numIn > 0 && in && in[0])
            std::copy(in[0], in[0] + blockLen_, buffer_.begin());
        else
            std::fill(buffer_.begin(), buffer_.begin() + blockLen_, 0.0f);
    }
    int channel() const override {
        return (int) params.get("Channel", default_ + 1.0) - 1;
    }
    int blockLength() const override { return blockLen_; }
    const float* channelData() const override { return buffer_.data(); }

private:
    int default_ = 2;
    std::vector<float> buffer_;
    int blockLen_ = 0;
};

}
