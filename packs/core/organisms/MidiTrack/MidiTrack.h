// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <mutex>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class MidiTrack : public Organism, public MidiNode, public LiveMidiIn {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void reset() override {
        stagedCount_ = 0;
        std::lock_guard<std::mutex> g(m_);
        liveCount_ = 0;
    }
    void process(const float* const*, int, float* const*, int, int,
                 const Transport&) override {}

    int liveMidiPort() const override { return -1; }
    void pushLiveMidi(const MidiEvent& e) override {
        std::lock_guard<std::mutex> g(m_);
        if (liveCount_ < (int) live_.size()) live_[(size_t) liveCount_++] = e;
    }

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent* out, int capacity) override {
        int n = std::min(stagedCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = staged_[(size_t) i];
        stagedCount_ = 0;
        std::unique_lock<std::mutex> g(m_, std::try_to_lock);
        if (g.owns_lock()) {
            for (int i = 0; i < liveCount_ && n < capacity; ++i) {
                out[n] = live_[(size_t) i];
                out[n].sampleOffset = 0;
                ++n;
            }
            liveCount_ = 0;
        }
        return n;
    }

private:
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::mutex m_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> live_;
    int liveCount_ = 0;
};

}
