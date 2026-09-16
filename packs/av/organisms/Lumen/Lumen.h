// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"

namespace hum {

class Lumen : public Organism, public VisualSource, public VideoNode {
public:
    static constexpr int kRing = 8192;
    static constexpr int kVideoIns = 4;

    Lumen() : ring_((size_t) kRing, 0.0f) {}

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override {
        std::fill(ring_.begin(), ring_.end(), 0.0f);
        wpos_.store(0, std::memory_order_relaxed);
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    void setVisualTapEnabled(bool on) override {
        tap_.store(on, std::memory_order_relaxed);
    }
    int readVisualTap(float* dest, int maxSamples) const override;

    int numVideoInputs() const override { return kVideoIns; }
    int numVideoOutputs() const override { return 1; }

private:
    std::vector<float> ring_;
    std::atomic<int> wpos_{0};
    std::atomic<bool> tap_{false};
};

}
