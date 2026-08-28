#pragma once
#include <array>
#include <vector>

#include "hum/dsp/Biquad.h"
#include "hum/Organism.h"

namespace hum {

class ParaEQ : public Organism {
public:
    explicit ParaEQ(int channels) : ch_(channels), bands_((size_t) channels) {}
    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }
    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    int ch_;
    std::vector<std::array<Biquad, 4>> bands_;
};

}
