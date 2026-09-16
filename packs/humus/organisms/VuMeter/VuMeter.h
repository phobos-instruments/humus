// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Audio.h"
#include "hum/Organism.h"

namespace hum {

class VuMeter : public Organism, public VuSource {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void reset() override {
        for (auto& a : rms_) a.store(0.0f, std::memory_order_relaxed);
        for (auto& p : peak_) p.store(0.0f, std::memory_order_relaxed);
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int vuChannels() const override { return 2; }
    float vuRms(int channel) const override {
        return rms_[channel & 1].load(std::memory_order_relaxed);
    }
    float vuPeak(int channel) const override {
        return peak_[channel & 1].load(std::memory_order_relaxed);
    }

private:
    std::atomic<float> rms_[2] = {0.0f, 0.0f};
    std::atomic<float> peak_[2] = {0.0f, 0.0f};
};

}
