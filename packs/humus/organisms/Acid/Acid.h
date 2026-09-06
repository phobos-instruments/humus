#pragma once
#include <array>
#include <mutex>

#include "hum/dsp/BlepOsc.h"
#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/Coupling303.h"
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
        coupling_.prepare(sampleRate_);
        lfo_.reset();
        lfoPrevPhase_ = 1.0;
        heldCount_ = 0;
        gate_ = false;
        envT_ = -1;
        amp_ = 0.0;
        accSweep_ = 0.0;
        lastOut_ = 0.0f;
        accentLevel_ = 0.0;
        squelchSet_ = -1.0;
        muffled_ = 0.0;
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
    static constexpr int kMaxHeld = 16;
    void noteOn(int note, int vel, const Tuning& tuning);
    void noteOff(int note, const Tuning& tuning);
    void forgetHeld(int note);
    double accentFor(int vel) const;

    BlepOsc osc_;
    DiodeLadder303 lp_;
    Coupling303 coupling_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;

    int note_ = -1;
    std::array<int, kMaxHeld> heldNotes_{};
    int heldCount_ = 0;
    bool gate_ = false;
    static constexpr int kAccentVelocity = 110;
    static constexpr double kSquelchLowHz = 100.0, kSquelchHighHz = 350.0;
    static constexpr double kMufflerOpenHz = 20000.0, kMufflerClosedHz = 1000.0;
    static constexpr double kSlideTimeConstant = 0.2;
    double accentLevel_ = 0.0;
    double squelchSet_ = -1.0;
    double muffled_ = 0.0;
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
