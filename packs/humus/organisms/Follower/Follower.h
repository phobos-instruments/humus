#pragma once
#include <atomic>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/EnvelopeFollower.h"

namespace hum {

class Follower : public Organism, public ControlSource {
public:
    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 1; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { env_.reset(); }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"env", ctl_.load(std::memory_order_relaxed)};
        return 1;
    }

private:
    EnvelopeFollower env_;
    std::atomic<float> ctl_{0.0f};
};

}
