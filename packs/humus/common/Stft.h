// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <juce_dsp/juce_dsp.h>

namespace hum {

class Stft {
public:
    using SpectrumFn = std::function<void(float* reim, int fftSize)>;

    void prepare(int order, int) {
        order_ = std::max(4, order);
        n_ = 1 << order_;
        hop_ = n_ / 4;
        fft_ = std::make_unique<juce::dsp::FFT>(order_);

        window_.resize((size_t) n_);
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (int i = 0; i < n_; ++i)
            window_[(size_t) i] = 0.5f - 0.5f * std::cos(twoPi * (float) i / (float) n_);

        analysis_.assign((size_t) n_, 0.0f);
        accum_.assign((size_t) n_, 0.0f);
        fftBuf_.assign((size_t) (2 * n_), 0.0f);
        stage_.assign((size_t) hop_, 0.0f);
        outQ_.assign((size_t) (2 * n_), 0.0f);
        olaScale_ = 1.0f / 1.5f;
        reset();
    }

    void reset() {
        std::fill(analysis_.begin(), analysis_.end(), 0.0f);
        std::fill(accum_.begin(), accum_.end(), 0.0f);
        std::fill(outQ_.begin(), outQ_.end(), 0.0f);
        stagePos_ = 0;
        qHead_ = qTail_ = qCount_ = 0;
    }

    int latencySamples() const { return n_; }
    int fftSize() const { return n_; }
    int hop() const { return hop_; }

    void setSpectrumFn(SpectrumFn fn) { onSpectrum_ = std::move(fn); }

    void process(const float* in, float* out, int n) {
        for (int i = 0; i < n; ++i) {
            stage_[(size_t) stagePos_++] = in[i];
            out[i] = qCount_ > 0 ? popOut() : 0.0f;
            if (stagePos_ == hop_) {
                stagePos_ = 0;
                frame();
            }
        }
    }

private:
    float popOut() {
        const float v = outQ_[(size_t) qHead_];
        qHead_ = (qHead_ + 1) % (int) outQ_.size();
        --qCount_;
        return v;
    }
    void pushOut(float v) {
        outQ_[(size_t) qTail_] = v;
        qTail_ = (qTail_ + 1) % (int) outQ_.size();
        ++qCount_;
    }

    void frame() {
        std::move(analysis_.begin() + hop_, analysis_.end(), analysis_.begin());
        std::copy(stage_.begin(), stage_.end(), analysis_.end() - hop_);

        for (int i = 0; i < n_; ++i)
            fftBuf_[(size_t) i] = analysis_[(size_t) i] * window_[(size_t) i];
        fft_->performRealOnlyForwardTransform(fftBuf_.data());
        if (onSpectrum_) onSpectrum_(fftBuf_.data(), n_);
        fft_->performRealOnlyInverseTransform(fftBuf_.data());

        for (int i = 0; i < n_; ++i)
            accum_[(size_t) i] += fftBuf_[(size_t) i] * window_[(size_t) i];

        for (int i = 0; i < hop_; ++i)
            pushOut(accum_[(size_t) i] * olaScale_);
        std::move(accum_.begin() + hop_, accum_.end(), accum_.begin());
        std::fill(accum_.end() - hop_, accum_.end(), 0.0f);
    }

    int order_ = 10, n_ = 1024, hop_ = 256;
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::vector<float> window_, analysis_, accum_, fftBuf_, stage_, outQ_;
    int stagePos_ = 0, qHead_ = 0, qTail_ = 0, qCount_ = 0;
    float olaScale_ = 1.0f / 1.5f;
    SpectrumFn onSpectrum_;
};

}
