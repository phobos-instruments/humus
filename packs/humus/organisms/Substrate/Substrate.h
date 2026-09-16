// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <mutex>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/PitchBend.h"
#include "hum/dsp/Biquad.h"
#include "hum/dsp/BlepOsc.h"
#include "hum/dsp/Lfo.h"

namespace hum {

class Substrate : public Organism, public MidiNode, public LiveMidiIn {
public:
    static constexpr int kMaxLayers = 64;
    static constexpr int kVoices = 8;
    static constexpr int kMaxStack = 8;
    static constexpr double kPhaseStride = 0.6180339887498949;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override;
    void reset() override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    int liveMidiPort() const override { return -1; }
    void pushLiveMidi(const MidiEvent& e) override {
        std::lock_guard<std::mutex> g(liveLock_);
        if (liveCount_ < (int) liveQ_.size()) liveQ_[(size_t) liveCount_++] = e;
    }

    void process(const float* const*, int, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    double smoothedInterval() const { return intervalSm_; }

private:
    struct Stratum {
        BlepOsc osc;
        Lfo pitchDrift, ampDrift;
    };
    struct Voice {
        std::array<Stratum, kMaxLayers> strata;
        int note = -1;
        bool gate = false;
        double env = 0.0;
    };
    std::array<Voice, kVoices> voices_;
    Biquad lpL_, lpR_;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;

    int midiRoot_ = -1;
    int held_ = 0;

    double intervalSm_ = -1.0;

    std::array<int, kVoices> heldNotes_{};
    int heldCount_ = 0;
    std::array<int, kVoices> lastNotes_{};
    int lastCount_ = 0;

    int next_ = 0;
    PitchBend bend_;
};

}
