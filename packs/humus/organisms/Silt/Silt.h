#pragma once
#include <array>
#include <mutex>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

#include "sid.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Silt : public Organism, public MidiNode, public LiveMidiIn {
public:
    static constexpr double kClock = 985248.0;
    static constexpr int kPerChip = 3;
    static constexpr float kTwinBleed = 0.35f;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int maxBlock) override;
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

    static int freqRegister(double hz);

private:
    bool twin() const { return params.get("Chips", 0.0) >= 0.5; }
    bool chained() const { return twin() && params.get("Chain", 0.0) >= 0.5; }
    void poke(int chip, int reg, int value);
    void applyGlobals();
    void applyVoiceShape(int chip, int v);
    void applyVoicePitch(int chip, int v);
    void applyVoiceControl(int chip, int v, bool gate);
    void noteOn(int note, int vel, const Tuning& tuning);
    void noteOff(int note);
    void renderChunk(float* l, float* r, int n, float level);

    std::array<reSID::SID, 2> sid_;
    double sampleRate_ = kDefaultSampleRate;
    std::array<int, 2> model_{-1, -1};
    std::array<float, 2> declick_{1.0f, 1.0f};

    std::array<int, kPerChip> voiceNote_{};
    std::array<double, kPerChip> voiceHz_{};
    std::array<unsigned, kPerChip> voiceAge_{};
    unsigned age_ = 0;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
};

}
