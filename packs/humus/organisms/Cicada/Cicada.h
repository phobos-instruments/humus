#pragma once
#include <array>
#include <cstdint>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class Cicada : public Organism, public MidiNode {
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
        for (auto& p : phase_) p = 0.0;
        cEnv_ = oEnv_ = crEnv_ = splash_ = 0.0;
        cAge_ = oAge_ = crAge_ = 0;
        choked_ = false;
        bz1_ = bz2_ = hz1_ = hz2_ = 0.0;
        lastC_ = lastO_ = lastCr_ = false;
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
    std::array<double, 6> phase_{};
    double cEnv_ = 0.0, oEnv_ = 0.0;
    double crEnv_ = 0.0, splash_ = 0.0;
    long cAge_ = 0, oAge_ = 0, crAge_ = 0;
    bool choked_ = false;
    double bz1_ = 0.0, bz2_ = 0.0;
    double hz1_ = 0.0, hz2_ = 0.0;
    bool lastC_ = false, lastO_ = false, lastCr_ = false;
    std::uint32_t rng_ = 0xC1CADA75u;
};

}
