// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hum/Organism.h"
#include "hum/caps/Midi.h"

namespace hum {

class MidiBus : public Organism, public MidiNode {
public:
    explicit MidiBus(int numInputs) : numInputs_(numInputs) {}

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return numInputs_; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    void deliverMidi(int port, const MidiEvent* events, int count) override {
        if (port == 0) count_ = 0;
        for (int i = 0; i < count && count_ < kMaxMidiEventsPerBlock; ++i)
            merged_[count_++] = events[i];
    }

    int collectMidi(int, MidiEvent* out, int capacity) override {
        for (int i = 1; i < count_; ++i) {
            const MidiEvent e = merged_[i];
            int j = i - 1;
            for (; j >= 0 && merged_[j].sampleOffset > e.sampleOffset; --j)
                merged_[j + 1] = merged_[j];
            merged_[j + 1] = e;
        }
        const int n = count_ < capacity ? count_ : capacity;
        for (int i = 0; i < n; ++i) out[i] = merged_[i];
        count_ = 0;
        return n;
    }

private:
    int numInputs_;
    MidiEvent merged_[kMaxMidiEventsPerBlock];
    int count_ = 0;
};

}
