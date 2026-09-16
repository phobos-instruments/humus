// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <atomic>

#include "hum/caps/Graph.h"
#include "hum/caps/Midi.h"
#include "hum/HeldNotes.h"
#include "hum/Organism.h"
#include "hum/dsp/DynamicsCore.h"

namespace hum {

class Gate : public Organism, public ControlSource, public MidiNode {
public:
    explicit Gate(int channels) : ch_(channels) {}
    int numAudioInputs() const override { return 2 * ch_; }
    int numAudioOutputs() const override { return ch_; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override {
        core_.reset();
        held_.clear();
        rms2_ = 0.0;
        openEnv_ = 0.0f;
        stagedCount_ = 0;
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"open", ctl_.load(std::memory_order_relaxed)};
        return 1;
    }

private:
    int ch_;
    GateCore core_;
    HeldNotes held_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    double rms2_ = 0.0;
    float openEnv_ = 0.0f;
    std::atomic<float> ctl_{0.0f};
};

}
