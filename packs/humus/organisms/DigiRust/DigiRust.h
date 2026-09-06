#pragma once
#include <cstdint>

#include "hum/Organism.h"
#include "hum/dsp/DcBlock.h"
#include "hum/dsp/EnvelopeFollower.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class DigiRust : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    float frand() {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return (float) (rng_ & 0xFFFFFF) / (float) 0xFFFFFF;
    }

    double sampleRate_ = kDefaultSampleRate;
    double phase_ = 0.0;
    double period_ = 1.0;
    float hold_[2] = {};
    float tone_[2] = {};
    EnvelopeFollower env_;
    DcBlock dc_[2];
    std::uint32_t rng_ = 0xD161D05u;
};

}
