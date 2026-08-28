#pragma once
#include "hum/Organism.h"
#include "hum/dsp/GainShape.h"

namespace hum {

class SideKick : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        g_ = 1.0;
        cachedText_.clear();
        shape_ = gainShapePreset(0);
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    GainShape shape_;
    std::string cachedText_;
    double g_ = 1.0;
};

}
