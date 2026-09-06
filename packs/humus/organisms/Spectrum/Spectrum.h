#pragma once
#include <atomic>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Spectrum : public Organism, public ScopeSource {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        for (auto& s : ring_) s = 0.0f;
        write_.store(0, std::memory_order_relaxed);
    }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int scopeRead(float* dst, int maxSamples) const override {
        const int n = maxSamples < kScopeSamples ? maxSamples : kScopeSamples;
        const int w = write_.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
            dst[i] = ring_[(w + kScopeSamples - n + i) % kScopeSamples];
        return n;
    }
    unsigned scopeStamp() const override { return stamp_.load(std::memory_order_relaxed); }
    double scopeRate() const override { return sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate; }

private:
    float ring_[kScopeSamples] = {};
    std::atomic<int> write_{0};
    std::atomic<unsigned> stamp_{0};
};

}
