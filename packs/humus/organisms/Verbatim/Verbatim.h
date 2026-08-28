#pragma once
#include <atomic>
#include <string>

#include <juce_dsp/juce_dsp.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/DelayLine.h"

namespace hum {

class Verbatim : public Organism, public FileLoader, public ReloadOnParam {
public:
    static constexpr double kMaxIrSeconds = 15.0;
    static constexpr double kMaxPredelayMs = 250.0;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string& uri) override;
    bool reloadsOn(const std::string& param) const override { return param == "Reverse"; }

private:
    juce::dsp::Convolution conv_{juce::dsp::Convolution::NonUniform{1024}};
    juce::AudioBuffer<float> scratch_;
    DelayLine pre_[2];
    double lp_[2] = {0.0, 0.0};
    double hp_[2] = {0.0, 0.0};
    double mixSm_ = 0.0, preSm_ = 0.0;
    bool smSnap_ = true;
    std::atomic<bool> irSet_{false};
    std::string lastPath_;
    int maxBlock_ = 0;
};

}
