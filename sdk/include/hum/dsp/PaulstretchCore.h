#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "hum/dsp/DspMath.h"

namespace hum {

class PaulstretchFrame {
public:
    static constexpr int kMinOrder = 9;
    static constexpr int kMaxOrder = 17;

    void prepare(int maxOrder) {
        maxOrder_ = std::clamp(maxOrder, kMinOrder, kMaxOrder);
        const int maxWin = 1 << maxOrder_;
        window_.resize((size_t) maxWin);
        fftBuf_.assign((size_t) (2 * maxWin), 0.0f);
        old_.assign((size_t) maxWin, 0.0f);
        for (int o = kMinOrder; o <= maxOrder_; ++o)
            if (!ffts_[(size_t) o]) ffts_[(size_t) o] = std::make_unique<juce::dsp::FFT>(o);
        order_ = 0;
        setOrder(kMinOrder);
    }

    void reset() { std::fill(old_.begin(), old_.end(), 0.0f); }

    void setOrder(int order) {
        order = std::clamp(order, kMinOrder, maxOrder_);
        if (order == order_) return;
        order_ = order;
        win_ = 1 << order;
        for (int i = 0; i < win_; ++i) {
            const double x = 2.0 * i / (double) (win_ - 1) - 1.0;
            window_[(size_t) i] = (float) std::pow(std::max(0.0, 1.0 - x * x), 1.25);
        }
        std::fill(old_.begin(), old_.begin() + win_, 0.0f);
    }

    int win() const { return win_; }
    int half() const { return win_ / 2; }

    void synth(const float* src, std::int64_t len, std::int64_t start, bool loop,
               std::uint32_t& rng, float* outHalf) {
        std::fill(fftBuf_.begin(), fftBuf_.begin() + 2 * win_, 0.0f);
        for (int i = 0; i < win_; ++i) {
            std::int64_t p = start + i;
            if (loop && len > 0) p %= len;
            fftBuf_[(size_t) i] =
                (src && p < len ? src[p] : 0.0f) * window_[(size_t) i];
        }
        ffts_[(size_t) order_]->performRealOnlyForwardTransform(fftBuf_.data());
        for (int k = 1; k < win_ / 2; ++k) {
            const float re = fftBuf_[(size_t) (2 * k)];
            const float im = fftBuf_[(size_t) (2 * k + 1)];
            const float mag = std::sqrt(re * re + im * im);
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            const double ph = kTwoPi * (double) rng / 4294967296.0;
            fftBuf_[(size_t) (2 * k)] = mag * (float) std::cos(ph);
            fftBuf_[(size_t) (2 * k + 1)] = mag * (float) std::sin(ph);
        }
        ffts_[(size_t) order_]->performRealOnlyInverseTransform(fftBuf_.data());
        for (int i = 0; i < win_; ++i) fftBuf_[(size_t) i] *= window_[(size_t) i];
        const int h = win_ / 2;
        for (int i = 0; i < h; ++i)
            outHalf[i] = std::clamp(
                fftBuf_[(size_t) i] + old_[(size_t) (h + i)], -1.0f, 1.0f);
        std::copy(fftBuf_.begin(), fftBuf_.begin() + win_, old_.begin());
    }

private:
    std::array<std::unique_ptr<juce::dsp::FFT>, kMaxOrder + 1> ffts_;
    int maxOrder_ = kMaxOrder;
    int order_ = 0;
    int win_ = 0;
    std::vector<float> window_, fftBuf_, old_;
};

inline int paulstretchOrderForSeconds(double seconds, double sampleRate) {
    const double target = std::max(16.0, seconds * sampleRate);
    int order = PaulstretchFrame::kMinOrder;
    while (order < PaulstretchFrame::kMaxOrder && (double) (1 << order) < target)
        ++order;
    if (order > PaulstretchFrame::kMinOrder
        && target - (double) (1 << (order - 1)) < (double) (1 << order) - target)
        --order;
    return order;
}

inline juce::AudioBuffer<float> paulstretchRender(const juce::AudioBuffer<float>& src,
                                                  double sampleRate, double stretch,
                                                  double windowSeconds = 0.25) {
    stretch = std::clamp(stretch, 1.0, 100.0);
    const int order = paulstretchOrderForSeconds(windowSeconds, sampleRate);
    const int channels = std::max(1, src.getNumChannels());
    const std::int64_t len = src.getNumSamples();
    if (len < 1) return {};

    std::vector<PaulstretchFrame> frames((size_t) channels);
    for (auto& f : frames) {
        f.prepare(order);
        f.setOrder(order);
    }
    const int half = frames[0].half();
    const double displace = (double) frames[0].win() * 0.5 / stretch;
    std::uint32_t rng = 0x2545f491u;

    juce::AudioBuffer<float> out(channels,
                                 (int) ((double) len * stretch) + frames[0].win());
    out.clear();
    int at = 0;
    for (double pos = 0.0; pos < (double) len && at + half <= out.getNumSamples();
         pos += displace, at += half)
        for (int c = 0; c < channels; ++c)
            frames[(size_t) c].synth(src.getReadPointer(c), len, (std::int64_t) pos,
                                     false, rng, out.getWritePointer(c) + at);
    out.setSize(channels, at, true, true, false);
    return out;
}

}
