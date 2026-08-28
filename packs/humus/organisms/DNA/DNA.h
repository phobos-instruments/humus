#pragma once
#include <array>
#include <cstdint>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/Chord.h"
#include "hum/Scale.h"

namespace hum {

class DNA : public Organism, public MidiNode {
public:
    static constexpr int kBases = 16;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        outCount_ = 0;
        offCount_ = 0;
        lastSeed_ = -1;
        lastBar_ = -1;
    }

    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        outCount_ = 0;
        return n;
    }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    const std::array<int, kBases>& genome() const { return genome_; }

private:
    void emit(int offset, bool on, int note, int vel);
    std::uint32_t rnd() {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return rng_;
    }
    void grow(int seed);

    std::array<int, kBases> genome_{};
    std::uint32_t rng_ = 1u;
    int lastSeed_ = -1;
    long lastBar_ = -1;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;
    struct PendingOff { int note; long samplesLeft; };
    std::array<PendingOff, 32> offs_;
    int offCount_ = 0;
};

}
