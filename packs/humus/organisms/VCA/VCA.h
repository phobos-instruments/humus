#pragma once
#include "hum/Organism.h"

namespace hum {

class Vca : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 1; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
};

}
