// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstdint>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class Kick : public Organism, public MidiNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 1; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        t_ = -1;
        phase_ = 0.0;
        wphase1_ = wphase2_ = 0.0;
        lastTrigParam_ = false;
    }

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    float frand() {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return (float) (rng_ & 0xFFFFFF) / (float) 0x7FFFFF - 1.0f;
    }

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    long t_ = -1;
    double phase_ = 0.0;
    double wphase1_ = 0.0, wphase2_ = 0.0;
    bool lastTrigParam_ = false;
    std::uint32_t rng_ = 0x9E3779B9u;
};

}
