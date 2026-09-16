// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/dsp/EnvelopeFollower.h"

namespace hum {

class Follower : public Organism, public ControlSource, public LevelMeterSource, public MidiNode,
                 public PinKinds {
public:
    bool controlOutlet(int) const override { return true; }
    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 2) return 0;
        out[0] = {"env", ctl_.load(std::memory_order_relaxed)};
        out[1] = {"gate", gate_.load(std::memory_order_relaxed)};
        return 2;
    }

    int meterChannels() const override { return 1; }
    float meterLevel(int) const override { return ctl_.load(std::memory_order_relaxed); }

    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int capacity) override;

    bool gateOpen() const { return open_; }

private:
    void emit(int offset, bool on, int velocity);

    EnvelopeFollower env_;
    bool open_ = false;
    int holdLeft_ = 0;
    int soundingNote_ = -1;
    std::atomic<float> ctl_{0.0f};
    std::atomic<float> gate_{0.0f};
    MidiEvent pending_[8];
    int pendingCount_ = 0;
};

}
