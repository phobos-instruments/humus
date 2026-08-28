#pragma once
#include <algorithm>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/LevelMeter.h"

namespace hum {

class Mixer : public Organism, public LevelMeterSource, public WantsNullInlets {
public:
    Mixer(int numInputs, int width, bool pan = false)
        : numInputs_(numInputs), width_(width), pan_(pan) {}

    int numAudioInputs() const override { return numInputs_ * width_; }
    int numAudioOutputs() const override { return pan_ ? 2 : width_; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        meter_.prepare(sampleRate);
    }
    void reset() override { meter_.reset(); }

    int meterChannels() const override { return numInputs_; }
    float meterLevel(int strip) const override {
        if (strip < 0 || strip >= numInputs_) return 0.0f;
        float v = 0.0f;
        for (int side = 0; side < width_; ++side)
            v = std::max(v, meter_.level(strip * width_ + side));
        return v;
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    int numInputs_;
    int width_;
    bool pan_;
    LevelMeter meter_;
    std::string inputSuffix(int k) const;
};

}
