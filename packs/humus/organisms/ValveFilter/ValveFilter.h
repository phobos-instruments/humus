#pragma once
#include "hum/Organism.h"
#include "hum/dsp/SvfTpt.h"

namespace hum {

class ValveFilter : public Organism {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        for (auto& s : svf_) s.reset();
        for (auto& d : dcx_) d = 0.0;
        for (auto& d : dcy_) d = 0.0;
        env_ = 0.0;
        lfoPhase_ = 0.0;
        on_ = hpG_ = bpG_ = lpG_ = 0.0;
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

private:
    SvfTpt svf_[2];
    double dcx_[2] = {}, dcy_[2] = {};
    double env_ = 0.0;
    double lfoPhase_ = 0.0;
    double on_ = 0.0, hpG_ = 0.0, bpG_ = 0.0, lpG_ = 0.0;
    double g_ = 0.1, k_ = 1.4, vg_ = 1.0, norm_ = 1.0, hard_ = 0.0, asym_ = 0.1;
};

}
