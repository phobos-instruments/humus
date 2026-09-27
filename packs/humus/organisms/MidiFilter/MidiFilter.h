// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <string>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class MidiFilter : public Organism, public MidiNode {
public:
    static constexpr int kRules = 8;
    static constexpr int kChannels = 16;
    static constexpr int kNotes = 128;
    static constexpr int kInOff = 0, kInNote = 1, kInCC = 2;
    static constexpr int kDoBlock = 0, kDoNote = 1, kDoCC = 2;
    static constexpr int kOthersPass = 0, kOthersBlock = 1;
    static constexpr int kCCNoteThreshold = 64;

    MidiFilter();

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double, int) override { reset(); }
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        outCount_ = 0;
        return n;
    }

private:
    struct Rule {
        int in = kInOff;
        int low = 0, high = 0;
        int action = kDoBlock;
        int target = 0;
    };
    struct Route {
        enum Kind : unsigned char { kNone, kPass, kBlock, kNote, kCC };
        Kind kind = kNone;
        unsigned char number = 0;
    };

    void readRules();
    Route routeFor(int in, int number) const;
    void handle(const MidiEvent& e);
    void handleNote(const MidiEvent& e, int channel, int note, int velocity, bool on);
    void handleCC(const MidiEvent& e, int channel, int controller, int value);
    void emit(const MidiEvent& like, unsigned char status, int d1, int d2);
    void pass(const MidiEvent& e);

    std::array<Rule, kRules> rules_{};
    std::array<std::array<std::string, 5>, kRules> names_;
    int others_ = kOthersPass;

    std::array<std::array<Route, kNotes>, kChannels> held_{};
    std::array<std::array<short, kNotes>, kChannels> ccNote_{};

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;
};

}
