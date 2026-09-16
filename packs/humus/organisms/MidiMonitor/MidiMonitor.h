// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class MidiMonitor : public Organism, public MidiNode, public LiveMidiIn,
                    public MidiLogSource {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent* events, int count) override {
        passCount_ = 0;
        for (int i = 0; i < count; ++i) {
            if (passCount_ < kMaxMidiEventsPerBlock) pass_[passCount_++] = events[i];
            cord_.push({events[i], 1, -1});
            generation_.fetch_add(1, std::memory_order_release);
        }
    }
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = passCount_ < capacity ? passCount_ : capacity;
        for (int i = 0; i < n; ++i) out[i] = pass_[i];
        passCount_ = 0;
        return n;
    }

    bool monitorsAllPorts() const override { return true; }
    void pushLiveMidi(const MidiEvent& e) override { pushLiveMidi(e, -1); }
    void pushLiveMidi(const MidiEvent& e, int port) override {
        live_.push({e, 0, (signed char) port});
        generation_.fetch_add(1, std::memory_order_release);
    }

    int consumeLog(Logged* dest, int maxEvents) override {
        int n = live_.drain(dest, maxEvents);
        n += cord_.drain(dest + n, maxEvents - n);
        return n;
    }
    unsigned logGeneration() const override {
        return generation_.load(std::memory_order_acquire);
    }

private:
    struct Ring {
        static constexpr unsigned kSize = 1024;
        Logged slots[kSize];
        std::atomic<unsigned> head{0};
        std::atomic<unsigned> tail{0};

        void push(const Logged& e) {
            const unsigned h = head.load(std::memory_order_relaxed);
            if (h - tail.load(std::memory_order_acquire) >= kSize) return;
            slots[h % kSize] = e;
            head.store(h + 1, std::memory_order_release);
        }
        int drain(Logged* dest, int maxEvents) {
            const unsigned h = head.load(std::memory_order_acquire);
            unsigned t = tail.load(std::memory_order_relaxed);
            int n = 0;
            while (t != h && n < maxEvents) dest[n++] = slots[t++ % kSize];
            tail.store(t, std::memory_order_release);
            return n;
        }
    };

    MidiEvent pass_[kMaxMidiEventsPerBlock];
    int passCount_ = 0;
    Ring live_, cord_;
    std::atomic<unsigned> generation_{0};
};

}
