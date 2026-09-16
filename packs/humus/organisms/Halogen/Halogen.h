// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "hum/caps/Files.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"
#include "hum/PixelField.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Halogen : public Organism, public FileLoader, public ControlSource, public VideoNode,
                public VideoFrameSink, public PixelFieldSource {
public:
    static constexpr int kVideoFieldW = 512;
    static constexpr int kVideoFieldH = 256;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int numVideoInputs() const override { return 1; }
    int numVideoOutputs() const override { return 0; }
    void pushVideoFrame(const Picture& picture) override;
    void setVideoCordAttached(bool on) override;
    bool videoAttachedForTest() const { return videoAttached_.load(std::memory_order_relaxed); }
    unsigned pixelFieldGeneration() const override { return displayGen_.load(std::memory_order_relaxed); }
    bool copyPixelField(PixelField& out) const override {
        const juce::SpinLock::ScopedLockType sl(displayLock_);
        out = display_;
        return !out.empty();
    }
    int fieldWidthForTest() const { return field_.width; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    void loadFromFile(const std::string& uri) override;

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"scan", scanOut_.load(std::memory_order_relaxed)};
        if (capacity < 2) return 1;
        out[1] = {"level", levelOut_.load(std::memory_order_relaxed)};
        return 2;
    }

    static constexpr int kOrder = 11;
    static constexpr int kSize = 1 << kOrder;
    static constexpr int kHop = kSize / 4;

private:
    void frame(double column);
    void adoptPending();
    void offerField(PixelField next, bool keepMagnitudes);

    double sampleRate_ = kDefaultSampleRate;
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::vector<float> window_, fftBuf_, mag_;
    std::vector<float> phase_[2], accum_[2];
    int outRead_ = kHop;

    PixelField field_;
    PixelField raw_;
    PixelField pending_;
    int blurRadius_ = -1;
    juce::SpinLock swap_;
    std::atomic<bool> ready_{false};
    bool softSwap_ = false;
    PixelField display_;
    mutable juce::SpinLock displayLock_;
    std::atomic<unsigned> displayGen_{0};
    std::atomic<bool> videoAttached_{false};
    std::string loaded_;

    double scan_ = 0.0;
    std::atomic<float> scanOut_{0.0f}, levelOut_{0.0f};
};

}
