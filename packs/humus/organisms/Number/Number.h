#pragma once
#include "hum/Organism.h"

namespace hum {

class Number : public Organism {
public:
    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 1; }
    void prepare(double sampleRate, int maxBlock) override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    float smoothed_ = 0.0f;
    float coef_ = 0.01f;
};

}
