#pragma once
#include <array>
#include <vector>

#include "hum/Organism.h"

namespace hum {

class FrequencyShifter : public Organism {
public:
    static constexpr int kM = 32;
    static constexpr int kN = 2 * kM + 1;
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    std::array<double, kN> h_{};
    std::array<std::array<float, kN>, 2> ring_{};
    int wr_ = 0;
    double phase_ = 0.0;
};

}
