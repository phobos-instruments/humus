#pragma once
#include <array>
#include <cstdint>
#include <mutex>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/Biquad.h"
#include "hum/dsp/BlepOsc.h"
#include "hum/dsp/Lfo.h"

namespace hum {

class Rhizome : public Organism, public MidiNode, public LiveMidiIn {
public:
    static constexpr int kVoices = 6;
    static constexpr int kMaxRunners = 8;

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

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    struct Runner {
        BlepOsc osc;
        Lfo creep;
    };
    struct Voice {
        int note = -1;
        float vel = 1.0f;
        bool gate = false;
        double env = 0.0;
        std::array<Runner, kMaxRunners> runners;
        std::array<double, kMaxRunners> hz{};
    };

    void noteOn(int note, int vel);
    void noteOff(int note);

    std::array<Voice, kVoices> voices_;
    int next_ = 0;
    Biquad lpL_, lpR_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
};

}
