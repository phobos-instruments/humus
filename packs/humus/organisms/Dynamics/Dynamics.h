#pragma once
#include <atomic>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/DynamicsCore.h"

namespace hum {

class Compressor : public Organism, public GainReductionSource {
public:
    explicit Compressor(int channels) : ch_(channels) {}
    float grDb() const override { return gr_.load(std::memory_order_relaxed); }
    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { core_.reset(); }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    int ch_;
    CompressorCore core_;
    std::atomic<float> gr_{0.0f};
};

class Limiter : public Organism, public GainReductionSource {
public:
    explicit Limiter(int channels) : ch_(channels) {}
    float grDb() const override { return gr_.load(std::memory_order_relaxed); }
    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { core_.reset(); }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    int ch_;
    LimiterCore core_;
    std::atomic<float> gr_{0.0f};
};

class NoiseGate : public Organism, public GainReductionSource {
public:
    explicit NoiseGate(int channels) : ch_(channels) {}
    float grDb() const override { return gr_.load(std::memory_order_relaxed); }
    int numAudioInputs() const override { return ch_; }
    int numAudioOutputs() const override { return ch_; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override { core_.reset(); }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;
private:
    int ch_;
    std::atomic<float> gr_{0.0f};
    GateCore core_;
};

}
