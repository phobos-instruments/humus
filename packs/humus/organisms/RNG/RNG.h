// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <string>

#include "common/NumberFormat.h"
#include "hum/caps/Graph.h"
#include "hum/Organism.h"

namespace hum {

class RNG : public Organism, public ControlSource, public PinKinds {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    bool controlOutlet(int) const override { return true; }
    bool controlInlet(const std::string& param) const override { return param == "Trigger"; }

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        const auto& s = numfmt::shapeOf(format_.load(std::memory_order_relaxed));
        out[0] = {"value", value_.load(std::memory_order_relaxed),
                  s.bounded ? (float) s.lo : 0.0f, s.bounded ? (float) s.hi : 0.0f};
        return 1;
    }

private:
    double sampleRate_ = 48000.0;
    double due_ = 0.0;
    bool triggerHeld_ = false;
    bool pending_ = true;
    std::uint64_t state_ = 0;

    std::atomic<float> value_{0.0f};
    std::atomic<int> format_{0};
};

}
