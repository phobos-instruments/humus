// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "hum/Organism.h"

namespace hum {

class Drops : public Organism {
public:
    static constexpr int kVoices = 24;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int) override;
    void reset() override;

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    float frand() {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return (float) (rng_ & 0xFFFFFF) / (float) 0xFFFFFF;
    }
    void trigger(double size, double spread, double chirp, float width);

    struct Voice {
        long t = -1;
        double phase = 0.0;
        double f0 = 0.0;
        double sigma = 0.0;
        double damp = 0.0;
        float amp = 0.0f;
        float gainL = 0.0f, gainR = 0.0f;
    };
    std::array<Voice, kVoices> voices_;

    std::vector<float> poolBufL_, poolBufR_;
    size_t poolPosL_ = 0, poolPosR_ = 0;
    float poolLpL_ = 0.0f, poolLpR_ = 0.0f;

    double nextDrop_ = 0.0;
    std::uint32_t rng_ = 0x5EEDD09u;
};

}
