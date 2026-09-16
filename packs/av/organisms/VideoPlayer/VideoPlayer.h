// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Midi.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"

namespace hum {

class VideoPlayer : public Organism, public VideoNode, public MidiNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double, int) override {}
    void reset() override {}
    void process(const float* const*, int, float* const*, int, int,
                 const Transport&) override {}

    int numVideoInputs() const override { return 0; }
    int numVideoOutputs() const override { return 1; }
    unsigned videoLaunchCount() const override {
        return launches_.load(std::memory_order_relaxed);
    }

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }
    void deliverMidi(int, const MidiEvent* events, int count) override {
        for (int i = 0; i < count; ++i) {
            const auto& e = events[i];
            if (e.size >= 3 && (e.data[0] & 0xF0) == 0x90 && e.data[2] != 0)
                launches_.fetch_add(1, std::memory_order_relaxed);
        }
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

private:
    std::atomic<unsigned> launches_{0};
};

}
