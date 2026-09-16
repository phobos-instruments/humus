// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <mutex>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class MidiInNode : public Organism, public MidiNode, public LiveMidiIn {
public:
    explicit MidiInNode(int port = 0) : default_(port) {}
    int liveMidiPort() const override { return (int) params.get("Port", default_ + 1.0) - 1; }
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent*, int) override {}

    void pushLiveMidi(const MidiEvent& e) override {
        std::lock_guard<std::mutex> g(m_);
        if (count_ < (int) buf_.size()) buf_[(size_t) count_++] = e;
    }

    int collectMidi(int, MidiEvent* out, int capacity) override {
        std::unique_lock<std::mutex> g(m_, std::try_to_lock);
        if (!g.owns_lock()) return 0;
        const int n = std::min(count_, capacity);
        for (int i = 0; i < n; ++i) out[i] = buf_[(size_t) i];
        count_ = 0;
        return n;
    }

private:
    const int default_;
    std::mutex m_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> buf_;
    int count_ = 0;
};

class MidiOutNode : public Organism, public MidiNode, public PendingMidiOut {
public:
    explicit MidiOutNode(int port = 0) : default_(port) {}
    int pendingMidiPort() const override { return (int) params.get("Port", default_ + 1.0) - 1; }
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    void deliverMidi(int, const MidiEvent* events, int count) override {
        int h = head_.load(std::memory_order_relaxed);
        const int t = tail_.load(std::memory_order_acquire);
        for (int i = 0; i < count; ++i) {
            const int next = (h + 1) % kRing;
            if (next == t) break;
            ring_[(size_t) h] = events[i];
            h = next;
        }
        head_.store(h, std::memory_order_release);
    }

    int consumeOutput(MidiEvent* out, int capacity) override {
        int t = tail_.load(std::memory_order_relaxed);
        const int h = head_.load(std::memory_order_acquire);
        int n = 0;
        while (t != h && n < capacity) { out[n++] = ring_[(size_t) t]; t = (t + 1) % kRing; }
        tail_.store(t, std::memory_order_release);
        return n;
    }

private:
    const int default_;
    static constexpr int kRing = 1024;
    std::array<MidiEvent, kRing> ring_;
    std::atomic<int> head_{0}, tail_{0};
};

}
