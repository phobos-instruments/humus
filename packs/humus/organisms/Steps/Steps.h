// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <vector>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {

class Steps : public Organism, public MidiNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        outCount_ = 0;
        offCount_ = 0;
    }

    void setPattern(const Pattern& p) override {
        pattern_ = p;
        stepsDirty_ = true;
    }

    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        setPattern(state.pattern);
    }

    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        outCount_ = 0;
        return n;
    }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    void emit(int offset, bool on, int note, int vel);

    Pattern pattern_;
    bool stepsDirty_ = true;
    std::vector<ArpStep> steps_;
    std::vector<bool> ups_;
    double stepsPerBeat_ = 4.0;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;
    struct PendingOff { int note; long samplesLeft; };
    std::array<PendingOff, 32> offs_;
    int offCount_ = 0;
};

}
