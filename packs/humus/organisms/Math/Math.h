// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

#include "hum/caps/Graph.h"
#include "hum/caps/Midi.h"
#include "hum/HeldNotes.h"
#include "hum/Organism.h"
#include "hum/dsp/Formula.h"
#include "hum/dsp/Prepared.h"

namespace hum {

class MathNode : public Organism, public ControlSource, public MidiNode, public PinKinds {
public:
    bool controlOutlet(int) const override { return true; }
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        syncExpression();
        reset();
    }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        syncExpression();
    }
    void onTextChanged(const std::string& param, const std::string& text) override;
    void reset() override {
        rng_ = 0x9e3779b9u;
        prev_.fill(0.0f);
        t_ = 0.0;
        beat_ = 0.0;
        for (auto& st : state_) { st.fill(0.0f); seedFormulaState(st.data(), prog_); }
        held_.clear();
        knobsPrimed_ = false;
        gateEnv_ = 0.0f;
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
        out[0] = {"out", ctl_.load(std::memory_order_relaxed)};
        return 1;
    }

private:
    static FormulaProgram compileExpression(const std::string& text, const FormulaProgram& last);
    void syncExpression();

    FormulaProgram prog_;
    Prepared<FormulaProgram> pendingProg_;
    std::string appliedText_;
    std::atomic<float> ctl_{0.5f};
    std::uint32_t rng_ = 0x9e3779b9u;
    std::array<std::array<float, FormulaProgram::kStateSlots>, 2> state_{};
    std::array<float, 2> prev_{};
    double t_ = 0.0, beat_ = 0.0;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    HeldNotes held_;
    int lastNote_ = 60;

    std::array<float, 5> knobs_{};
    bool knobsPrimed_ = false;
    float gateEnv_ = 0.0f;
};

}
