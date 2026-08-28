#pragma once
#include <array>

#include "hum/dsp/DelayLine.h"
#include "hum/dsp/Lfo.h"
#include "hum/Organism.h"

namespace hum {

class SChorus : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override;
    void reset() override { lines_[0].clear(); lines_[1].clear(); lp_ = {}; lfo_.reset(); }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    std::array<DelayLine, 2> lines_;
    std::array<double, 2> lp_{};
    Lfo lfo_;
};

}
