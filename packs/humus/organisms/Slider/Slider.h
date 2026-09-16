// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Graph.h"
#include "hum/Organism.h"

namespace hum {

class Slider : public Organism, public ControlSource, public PinKinds {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    bool controlOutlet(int) const override { return true; }

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"value", ctl_.load(std::memory_order_relaxed),
                  lo_.load(std::memory_order_relaxed), hi_.load(std::memory_order_relaxed)};
        return 1;
    }

private:
    float smoothed_ = 0.0f;
    bool settled_ = false;
    std::atomic<float> ctl_{0.0f};
    std::atomic<float> lo_{0.0f};
    std::atomic<float> hi_{1.0f};
};

}
