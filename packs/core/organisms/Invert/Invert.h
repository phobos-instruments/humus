#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class Invert : public Organism, public StereoFieldSource {
public:
    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }
    void configureChannels(int inlets, int) override { ch_ = std::max(1, inlets); }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override {
        for (int c = 0; c < numOut; ++c)
            for (int n = 0; n < numSamples; ++n)
                out[c][n] = -((c < numIn && in[c]) ? in[c][n] : 0.0f);
        if (numOut < 2) return;
        double sll = 0.0, srr = 0.0, slr = 0.0;
        unsigned w = widx_.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i) {
            const float L = out[0][i], R = out[1][i];
            ring_[2 * w] = L;
            ring_[2 * w + 1] = R;
            w = (w + 1) % (unsigned) kFieldPairs;
            sll += (double) L * L; srr += (double) R * R; slr += (double) L * R;
        }
        widx_.store(w, std::memory_order_relaxed);
        if (const double denom = std::sqrt(sll * srr); denom > 1e-12) {
            corr_.store((float) (slr / denom), std::memory_order_relaxed);
            stamp_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    int fieldRead(float* lr, int maxPairs) const override {
        const unsigned w = widx_.load(std::memory_order_relaxed);
        const int n = maxPairs < kFieldPairs ? maxPairs : kFieldPairs;
        for (int i = 0; i < n; ++i) {
            const unsigned idx = (w + kFieldPairs - n + (unsigned) i) % kFieldPairs;
            lr[2 * i] = ring_[2 * idx];
            lr[2 * i + 1] = ring_[2 * idx + 1];
        }
        return n;
    }
    unsigned fieldStamp() const override { return stamp_.load(std::memory_order_relaxed); }
    float fieldCorrelation() const override { return corr_.load(std::memory_order_relaxed); }

private:
    int ch_ = 2;
    float ring_[2 * kFieldPairs] = {};
    std::atomic<unsigned> widx_{0};
    std::atomic<unsigned> stamp_{0};
    std::atomic<float> corr_{0.0f};
};

}
