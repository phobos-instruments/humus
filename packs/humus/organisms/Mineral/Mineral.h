#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class Mineral : public Organism, public MidiNode, public LiveMidiIn,
                public SigilSource {
public:
    static constexpr int kVoices = 4;
    static constexpr int kMaxFacets = 12;
    static_assert(kMaxFacets + 2 <= SigilSource::kMaxPoints, "gem outgrew a Prim");

    static double geometricRatio(int facets);

    int sigil(Prim* out, int capacity, double timeSeconds) override;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        for (auto& v : voices_) v.t = -1;
        next_ = 0;
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
    struct Voice {
        long t = -1;
        double f0 = 220.0;
        float vel = 1.0f;
        std::array<double, kMaxFacets> phL{}, phR{};
    };
    std::array<Voice, kVoices> voices_;
    int next_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
    std::uint32_t rng_ = 0x2545F491u;

    std::atomic<float> sigFacets_{5.0f}, sigRatio_{1.62f}, sigShine_{0.7f},
                       sigDecay_{900.0f}, sigStrike_{0.4f}, sigShimmer_{0.35f};
    std::atomic<float> sigLevel_{0.0f};
    std::atomic<float> sigHit_{0.0f};
};

}
