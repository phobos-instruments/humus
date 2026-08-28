#pragma once
#include <atomic>
#include <vector>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/PitchTrack.h"

#include "common/Stft.h"

namespace hum {

class Prism : public Organism, public LatencyReporting, public PitchDetectSource {
public:
    static constexpr int kHarms = 16;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int latencySamples() const override {
        return chans_.empty() ? 0 : chans_.front().stft.latencySamples();
    }

    float detectedHz() const override { return pubHz_.load(std::memory_order_relaxed); }
    float detectClarity() const override { return pubClarity_.load(std::memory_order_relaxed); }
    float detectLevel() const override { return pubLevel_.load(std::memory_order_relaxed); }
    int detectedNote() const override { return pubNote_.load(std::memory_order_relaxed); }

private:
    struct Chan {
        Stft stft;
        std::vector<float> dry;
        int dryPos = 0;
    };
    struct Harm {
        double aPhase = 0.0;
        double sPhase = 0.0;
        double zr1 = 0.0, zi1 = 0.0, zr2 = 0.0, zi2 = 0.0;
        double amp = 0.0;
    };

    void onFrame(float* reim, int fftSize);

    int order_ = 10;
    std::vector<Chan> chans_;
    std::vector<float> wet_, mono_, bank_, silence_;
    std::vector<float> bankDelay_;
    int bankPos_ = 0;

    PitchTracker tracker_;
    double f0_ = 0.0;
    double f0Target_ = 0.0;
    double gate_ = 0.0;
    Harm harms_[kHarms];

    float frameF0_ = 0.0f;
    float frameGains_[kHarms] = {};
    float frameResidual_ = 1.0f;

    std::atomic<float> pubHz_{0.0f}, pubClarity_{0.0f}, pubLevel_{0.0f};
    std::atomic<int> pubNote_{-1};
};

}
