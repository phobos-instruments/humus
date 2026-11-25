// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "hum/BankedParams.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/Chord.h"
#include "hum/Scale.h"
#include "hum/StepChance.h"

namespace hum {

class DNA : public Organism, public MidiNode, public StepStrip {
public:
    static constexpr int kBases = 16;

    static std::uint32_t stepHash(std::int64_t idx, int seed) {
        std::uint32_t h = (std::uint32_t) (idx * 2246822519u) ^ (std::uint32_t) (seed * 374761393u);
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;
        return h;
    }

    int stripSteps() const override { return kBases; }
    std::int64_t stripStepAt(double beats) const override {
        return (std::int64_t) std::floor(beats * stripStepsPerBeat_.load(std::memory_order_relaxed));
    }
    bool stripStepRests(std::int64_t step) const override {
        const auto slot = (std::size_t) (((step % kBases) + kBases) % kBases);
        return (stepHash(step, stripSeed_.load(std::memory_order_relaxed)) & 0xFFFF) / 65536.0
                   < stripRest_.load(std::memory_order_relaxed)
               || !stepPlays(chanceSeed_.load(std::memory_order_relaxed), 0, (long) step,
                             kFullChance - stripMiss_[slot].load(std::memory_order_relaxed));
    }

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        chanceSeed_.store(stepSeedFor(name()), std::memory_order_relaxed);
        reset();
    }
    void setPattern(const Pattern& p) override { banked_.read(p); }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        setPattern(state.pattern);
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

    static constexpr std::size_t kSeedSlot = 0, kMuteSlot = 1, kBaseSlot = 2, kChanceSlot = 6;
    static std::array<ParamRef, kChanceSlot + kBases> bankedRefs() {
        std::array<ParamRef, kChanceSlot + kBases> refs;
        refs[kSeedSlot] = ParamRef("Seed");
        refs[kMuteSlot] = ParamRef("Mute");
        const char* bases[4] = {"Base A", "Base C", "Base G", "Base T"};
        for (std::size_t b = 0; b < 4; ++b) refs[kBaseSlot + b] = ParamRef(bases[b]);
        for (int s = 0; s < kBases; ++s) refs[kChanceSlot + (std::size_t) s] = ParamRef::numbered("Chance_", s + 1);
        return refs;
    }
    BankedParams<kChanceSlot + kBases> banked_{bankedRefs()};
    ParamRef bankRef_{"Bank"};
    std::atomic<std::uint32_t> chanceSeed_{0};
    std::array<std::atomic<int>, kBases> stripMiss_{};

    std::atomic<double> stripStepsPerBeat_{4.0};
    std::atomic<int> stripSeed_{1};
    std::atomic<double> stripRest_{0.25};

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
