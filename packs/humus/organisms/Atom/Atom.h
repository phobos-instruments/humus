// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

#include "Atom/RingRules.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/ParamRef.h"
#include "hum/Pattern.h"
#include "hum/StepChance.h"

namespace hum {

class Atom : public Organism, public MidiNode, public RingFace {
public:
    static constexpr int kRings = 4;
    static constexpr int kMaxSteps = 32;
    static constexpr int kBanks = 8;
    enum Rate { kQuarter = 0, kEighth, kSixteenth, kThirtySecond, kFitBar };

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override;
    void reset() override;
    void setPattern(const Pattern& p) override;
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        setPattern(state.pattern);
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        outCount_ = 0;
        return n;
    }

    int ringCount() const override { return kRings; }
    int ringSteps(int ring) const override;
    bool ringHits(int ring, int step) const override;
    int ringStepNow(int ring) const override { return now_[(size_t) ring].load(std::memory_order_relaxed); }
    unsigned ringFired(int ring) const override { return fired_[(size_t) ring].load(std::memory_order_relaxed); }

private:
    struct Ring {
        int steps = 16, hits = 0, rotate = 0, rate = kSixteenth, note = 60, chance = kFullChance;
        std::uint32_t muted = 0;
        bool fixed = false;
    };
    using Bank = std::array<Ring, kRings>;
    struct PendingOff { int note; long samplesLeft; };

    Ring ringAt(int ring) const;
    Ring paramRing(int ring) const;
    static Ring clamped(Ring shape);
    static void storeValue(Bank& bank, const std::string& name, long long value);
    void hear(bool latch);
    void releaseDue(int numSamples);
    void turn(int ring, const Ring& shape, const rings::Pool& pool, int numSamples, const Transport& transport);
    void strike(int ring, const Ring& shape, const rings::Pool& pool, int at, long length);
    void emit(int offset, bool on, int note, int velocity);
    void hush();

    std::array<ParamRef, kRings> stepsRef_ = numberedParams<kRings>("Steps_");
    std::array<ParamRef, kRings> hitsRef_ = numberedParams<kRings>("Hits_");
    std::array<ParamRef, kRings> rotateRef_ = numberedParams<kRings>("Rotate_");
    std::array<ParamRef, kRings> rateRef_ = numberedParams<kRings>("Rate_");
    std::array<ParamRef, kRings> fixedRef_ = numberedParams<kRings>("Fixed_");
    std::array<ParamRef, kRings> noteRef_ = numberedParams<kRings>("Note_");
    std::array<ParamRef, kRings> chanceRef_ = numberedParams<kRings>("Chance_");
    std::array<ParamRef, kRings> muteRef_ = numberedParams<kRings>("Mute_");
    ParamRef bankRef_{"Bank"};

    std::array<Bank, kBanks> banks_{};
    std::array<bool, kBanks> stored_{};
    std::uint32_t seed_ = 0;

    rings::HeldNotes held_;
    std::array<long long, kRings> turns_{};
    std::array<std::atomic<int>, kRings> now_{};
    std::array<std::atomic<unsigned>, kRings> fired_{};
    std::uint32_t random_ = 0x9e3779b9u;
    std::uint32_t seekSeen_ = 0;
    bool wasPlaying_ = false;
    bool wasLatched_ = false;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;
    std::array<PendingOff, 64> offs_;
    int offCount_ = 0;
};

}
