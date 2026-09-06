#pragma once
#include <atomic>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/GainShape.h"

namespace hum {

class SideKick : public Organism, public ControlSource {
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

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"gain", gainOut_.load(std::memory_order_relaxed)};
        if (capacity < 2) return 1;
        out[1] = {"phase", phaseOut_.load(std::memory_order_relaxed)};
        return 2;
    }

private:
    GainShape shape_;
    std::string cachedText_;
    double g_ = 1.0;
    std::atomic<float> gainOut_{1.0f};
    std::atomic<float> phaseOut_{0.0f};
};

}
