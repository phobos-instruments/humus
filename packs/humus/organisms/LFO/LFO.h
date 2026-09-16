// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Graph.h"
#include "hum/Organism.h"
#include "hum/dsp/Lfo.h"

namespace hum {

class LfoGen : public Organism, public ControlSource, public PinKinds {
public:
    bool controlOutlet(int) const override { return true; }
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { lfo_.reset(); prevPhase_ = 1.0; }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"wave", ctl_.load(std::memory_order_relaxed)};
        if (capacity < 2) return 1;
        out[1] = {"phase", phase_.load(std::memory_order_relaxed)};
        return 2;
    }

private:
    std::atomic<float> ctl_{0.5f};
    std::atomic<float> phase_{0.0f};
    Lfo lfo_;
    unsigned rng_ = 0x1234abcdu;
    double held_ = 0.0;
    double prevPhase_ = 1.0;
};

}
