// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>

#include "hum/caps/Midi.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"

namespace hum {

class Firefly : public Organism, public VideoNode, public VideoFxNode, public VideoFlashSource,
                public MidiNode {
public:
    static constexpr int kModeBeat = 0, kModeFree = 1, kModePlayed = 2;
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numVideoInputs() const override { return 1; }
    int numVideoOutputs() const override { return 1; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        count_.store(0);
        seconds_.store(0.05f);
        strength_.store(0.0f);
        held_ = 0.0;
        lastUnit_ = -1;
        wasPress_ = false;
        stagedCount_ = 0;
    }

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    void process(const float* const*, int, float* const*, int, int numSamples,
                 const Transport& transport) override;

    unsigned flashCount() const override { return count_.load(std::memory_order_relaxed); }
    float flashSeconds() const override { return seconds_.load(std::memory_order_relaxed); }
    float flashStrength() const override { return strength_.load(std::memory_order_relaxed); }
    void flashColour(float& r, float& g, float& b) const override {
        r = colourR_.load(std::memory_order_relaxed);
        g = colourG_.load(std::memory_order_relaxed);
        b = colourB_.load(std::memory_order_relaxed);
    }

    static void hueToRgb(double hueDegrees, double saturation, float& r, float& g, float& b);
    static double divisionBeats(int division);

private:
    void fire(double strength, double lengthSeconds);

    double sampleRate_ = 48000.0;
    double held_ = 0.0;
    long lastUnit_ = -1;
    bool wasPress_ = false;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::atomic<unsigned> count_{0};
    std::atomic<float> seconds_{0.05f}, strength_{0.0f};
    std::atomic<float> colourR_{1.0f}, colourG_{1.0f}, colourB_{1.0f};
};

}
