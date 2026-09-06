#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

#include "hum/dsp/DspMath.h"

namespace hum {

enum class GritCart { kNone, kVrc6, kMmc5, kFds, kN163, k5B, kVrc7 };

struct GritTarget {
    enum Kind { kPulse, kVrc6Pulse, kVrc6Saw, kMmc5Pulse, kFds, kN163, kFme7, kVrc7 };
    Kind kind = kPulse;
    int ch = 0;
};

class Grit : public Organism, public MidiNode, public LiveMidiIn {
public:
    static constexpr double kNtscClock = 1789772.0;
    static constexpr double kPalClock = 1662607.0;
    static constexpr int kMaxVoices = 8;
    static constexpr int kNoiseSplit = 96;
    static constexpr int kDpcmSplit = 36;
    static constexpr int kDpcmRate = 33143;

    Grit();
    ~Grit() override;

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

    double clock() const { return pal_ ? kPalClock : kNtscClock; }
    int voiceCount() const;
    GritTarget target(int voice) const;

    static std::vector<std::uint8_t> encodeDpcm(const float* mono, int count,
                                                double sourceRate);

private:
    struct Voice {
        int note = -1;
        double hz = 0.0;
        float vel = 1.0f;
        unsigned age = 0;
        int phase = 0;
        int level = 0;
        int wait = 0;
        int lastHi = -1;
        bool sounding = false;
    };

    struct Impl;

    GritCart cart() const {
        return (GritCart) std::clamp((int) params.get("Cartridge", 0.0), 0, 6);
    }
    void applyRegion();
    void applyCartSetup();
    void applyWavetables();
    void pokeApu(int reg, int value);
    void pokeExp(std::uint32_t adr, int value);
    void applyPitch(int v);
    void applyLevel(int v);
    void triggerVoice(int v);
    void silenceVoice(int v);
    void applyTriangle();
    void noteOn(int note, int vel, const Tuning& tuning);
    void noteOff(int note);
    void envelopeTick();
    void loadSample();
    void renderChunk(float* l, float* r, int n, float level);

    std::unique_ptr<Impl> impl_;

    double sampleRate_ = kDefaultSampleRate;
    bool pal_ = false;
    int cachedCart_ = -1;
    int cachedWave_ = -1;
    int cachedPatch_ = -1;
    int cachedDuty_ = -1;
    std::string cachedSample_;
    double cycleAcc_ = 0.0;
    double envAcc_ = 0.0;
    float dcIn_ = 0.0f;
    float dcOut_ = 0.0f;

    std::array<Voice, kMaxVoices> voices_;
    unsigned age_ = 0;
    int triNote_ = -1;
    int triHi_ = -1;
    int noiseNote_ = -1;
    Voice noiseEnv_;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
};

}
