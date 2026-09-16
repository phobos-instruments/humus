// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Audio.h"
#include "hum/Organism.h"
#include "hum/dsp/Biquad.h"

namespace hum {

class StereoTool : public Organism, public StereoFieldSource {
public:
    StereoTool() = default;
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { sideHp1_.reset(); sideHp2_.reset(); monoBassHz_ = -1.0; }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int fieldRead(float* lr, int maxPairs) const override {
        const unsigned w = widx_.load(std::memory_order_relaxed);
        const int n = maxPairs < kFieldPairs ? maxPairs : kFieldPairs;
        for (int i = 0; i < n; ++i) {
            const unsigned idx = (w + kFieldPairs - n + (unsigned) i) % kFieldPairs;
            lr[2 * i] = ring_[2 * idx];
            lr[2 * i + 1] = ring_[2 * idx + 1];
        }
        return n;
    }
    unsigned fieldStamp() const override { return stamp_.load(std::memory_order_relaxed); }
    float fieldCorrelation() const override { return corr_.load(std::memory_order_relaxed); }

private:
    Biquad sideHp1_, sideHp2_;
    double monoBassHz_ = -1.0;
    float ring_[2 * kFieldPairs] = {};
    std::atomic<unsigned> widx_{0};
    std::atomic<unsigned> stamp_{0};
    std::atomic<float> corr_{0.0f};
};

}
