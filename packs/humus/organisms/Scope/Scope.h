// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <vector>

#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/Organism.h"

namespace hum {

class Scope : public Organism, public TraceSource, public WantsNullInlets {
public:
    static constexpr int kRingSamples = 1 << 17;
    static constexpr int kRingMask = kRingSamples - 1;
    enum Mode { kStereo = 0, kMono, kLeftAgainstRight };

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    Shape traceShape() const override;
    int traceRead(float* left, float* right, int samples) const override;
    unsigned traceStamp() const override { return stamp_.load(std::memory_order_relaxed); }

private:
    int sweepSamples() const;

    std::vector<float> left_, right_;
    std::atomic<std::int64_t> written_{0};
    std::atomic<unsigned> stamp_{0};
    std::atomic<int> window_{960};
};

}
