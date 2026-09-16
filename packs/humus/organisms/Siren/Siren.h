// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>

#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class Siren : public Organism, public MidiNode, public ControlSource, public LevelMeterSource {
public:
    enum Mode { Sweep = 0, Rise = 1, Fall = 2, Step = 3 };
    enum Wave { Square = 0, Pulse = 1, Saw = 2 };

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 1; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        rising_ = true;
        seg_ = 0.0;
        sweep_ = 0.0;
        sweepSm_ = 0.0;
        phase_ = 0.0;
        lp_ = 0.0;
        env_ = 0.0;
        heldNotes_ = 0;
        wasGated_ = false;
        lfo_.store(0.0f);
        gate_.store(0.0f);
    }

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 2) return 0;
        out[0] = {"lfo", lfo_.load(std::memory_order_relaxed)};
        out[1] = {"gate", gate_.load(std::memory_order_relaxed)};
        return 2;
    }
    int meterChannels() const override { return 1; }
    float meterLevel(int) const override { return lfo_.load(std::memory_order_relaxed); }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    double advanceSweep(int mode, double up, double down, double dt);
    double oscillate(int wave, double inc);

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    bool rising_ = true;
    double seg_ = 0.0;
    double sweep_ = 0.0, sweepSm_ = 0.0;
    double phase_ = 0.0;
    double lp_ = 0.0;
    double env_ = 0.0;
    int heldNotes_ = 0;
    bool wasGated_ = false;
    std::atomic<float> lfo_{0.0f}, gate_{0.0f};
};

}
