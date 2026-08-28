#pragma once
#include <cstdint>
#include <vector>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

#include "common/Stft.h"

namespace hum {

class SpectralFreeze : public Organism, public LatencyReporting {
public:
    explicit SpectralFreeze(int channels) : ch_(channels) {}

    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int latencySamples() const override {
        return chans_.empty() ? 0 : chans_.front().stft.latencySamples();
    }

private:
    struct Chan {
        Stft stft;
        std::vector<float> heldMag, prevPhase, advance, outPhase, inMag, blur;
        std::vector<float> dry;
        int dryPos = 0;
        float fSmoothed = 0.0f;
    };

    void onFrame(Chan& c, float* reim, int fftSize);
    float nextRand() {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return (float) (rng_ & 0xFFFFFFu) / (float) 0x1000000u;
    }

    int ch_;
    std::vector<Chan> chans_;
    std::vector<float> wet_, silence_;
    double frameRate_ = 187.5;

    float smear_ = 0.0f, diffusion_ = 0.0f, shimmer_ = 0.0f, fTarget_ = 0.0f;
    float smearCoeff_ = 0.0f, fCoeff_ = 0.0f;
    bool frozen_ = false;
    std::uint32_t rng_ = 0x9E3779B9u;
};

}
