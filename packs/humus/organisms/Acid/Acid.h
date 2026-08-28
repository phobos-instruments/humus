#pragma once
#include <array>
#include <mutex>

#include "hum/dsp/BlepOsc.h"
#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/DiodeLadder303.h"
#include "hum/dsp/Lfo.h"

namespace hum {

class Acid : public Organism, public MidiNode, public LiveMidiIn {
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
        lp_.prepare(sampleRate_);
        lfo_.reset();
        lfoPrevPhase_ = 1.0;
        held_ = 0;
        gate_ = false;
        envT_ = -1;
        amp_ = 0.0;
        accSweep_ = 0.0;
        lastOut_ = 0.0f;
    }

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

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    void noteOn(int note, int vel, const Tuning& tuning);
    void noteOff(int note);

    BlepOsc osc_;
    DiodeLadder303 lp_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;

    int note_ = -1;
    int held_ = 0;
    bool gate_ = false;
    static constexpr int kAccentVelocity = 110;
    bool accent_ = false;
    long envT_ = -1;
    double curFreq_ = 110.0, targetFreq_ = 110.0;
    double amp_ = 0.0;
    double accSweep_ = 0.0;
    float lastOut_ = 0.0f;

    Lfo lfo_;
    unsigned lfoRng_ = 0x5eedcafeu;
    double lfoHeld_ = 0.0;
    double lfoPrevPhase_ = 1.0;
};

}
