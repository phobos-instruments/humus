#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/Biquad.h"
#include "hum/dsp/WaveTable.h"

namespace hum {

class Wave : public Organism, public MidiNode, public LiveMidiIn {
public:
    static constexpr int kVoices = 8;
    static constexpr int kLevels = 10;
    static constexpr int kMaxFrames = 16;

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
    struct Voice {
        int note = -1;
        float vel = 1.0f;
        bool gate = false;
        double env = 0.0;
        double phase = 0.0;
        double uphase[6] = {};
        double subPhase = 0.0;
        double hz = 0.0;
    };

    void noteOn(int note, int vel);
    void noteOff(int note);
    void rebuildTable(const std::string& text);
    int levelFor(double hz, double sr) const {
        const double maxPartial = 0.45 * sr / std::max(1.0, hz);
        int level = 0;
        while (level < kLevels - 1 && (double) (kWaveTableLen / 2 >> level) > maxPartial)
            ++level;
        return level;
    }

    float frames_[kMaxFrames][kWaveTableLen] = {};
    float frameMip_[kMaxFrames][kLevels][kWaveTableLen] = {};
    int frameCount_ = 1;
    float frameFft_[kMaxFrames][2 * kWaveTableLen] = {};
    float morphMip_[kLevels][kWaveTableLen] = {};
    int morphF0_ = -1, morphF1_ = -1;
    float morphFr_ = -1.0f;
    void buildMorph(int f0, int f1, float fr);
    bool tableReady_ = false;
    std::string cachedText_;
    std::unique_ptr<juce::dsp::FFT> fft_;
    float fwd_[2 * kWaveTableLen] = {};
    float lvl_[2 * kWaveTableLen] = {};

    std::array<Voice, kVoices> voices_;
    int next_ = 0;
    double freePhase_ = 0.0;
    double freeSubPhase_ = 0.0;
    double freeEnv_ = 0.0;
    Biquad lp_, lpR_;
    double lpHz_ = -1.0, lpQ_ = -1.0;
    std::vector<float> warpRamp_;
    double warpSm_ = 0.0;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
};

}
