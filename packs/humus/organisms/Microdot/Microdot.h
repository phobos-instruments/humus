#pragma once
#include <array>
#include <vector>

#include "hum/Capabilities.h"
#include "hum/HeldNotes.h"
#include "hum/Organism.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"
#include "hum/dsp/BlepOsc.h"
#include "hum/dsp/Biquad.h"

namespace hum {

class Microdot : public Organism, public MidiNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 1; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        osc_.reset();
        osc2_.reset(0.31);
        sub_.reset();
        lp_.reset();
        t_ = -1;
        stagedCount_ = 0;
        held_.clear();
    }

    void setPattern(const Pattern& p) override {
        pattern_ = p;
        stepsDirty_ = true;
    }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        setPattern(state.pattern);
    }

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    Pattern pattern_;
    bool stepsDirty_ = true;
    std::vector<ArpStep> steps_;
    std::vector<bool> ups_;
    double stepsPerBeat_ = 4.0;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    HeldNotes held_;

    BlepOsc osc_, osc2_, sub_;
    Biquad lp_;
    long t_ = -1;
    long gateLen_ = 0;
    int curNote_ = 33;
};

}
