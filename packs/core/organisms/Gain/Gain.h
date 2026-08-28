#pragma once
#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/LevelMeter.h"

namespace hum {

class Gain : public Organism, public LevelMeterSource {
public:
    explicit Gain(int channels) : channels_(channels) {}

    int numAudioInputs() const override { return channels_; }
    int numAudioOutputs() const override { return channels_; }

    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; meter_.prepare(sampleRate); }

    int meterChannels() const override { return channels_; }
    float meterLevel(int ch) const override { return meter_.level(ch); }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    int channels_;
    LevelMeter meter_;
};

}
