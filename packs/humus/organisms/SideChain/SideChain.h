#pragma once
#include "hum/Organism.h"
#include "hum/dsp/DynamicsCore.h"

namespace hum {

class SideChain : public Organism {
public:
    explicit SideChain(int channels) : ch_(channels) {}
    int numAudioInputs() const override { return 2 * ch_; }
    int numAudioOutputs() const override { return ch_; }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        core_.reset();
        det_ = 0.0;
        detHold_ = 0;
        ms_ = 0.0;
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    int ch_;
    CompressorCore core_;
    double det_ = 0.0;
    int detHold_ = 0;
    double ms_ = 0.0;
};

}
