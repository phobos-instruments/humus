// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstddef>

#include "hum/dsp/DspMath.h"

namespace hum {

class HalfBandStage {
public:
    static constexpr int kHalf = 16;
    static constexpr int kOddTaps = kHalf;
    static constexpr int kLatency = kHalf / 2;
    static constexpr int kRing = 2 * kHalf;

    HalfBandStage() { taps(); }

    void reset() {
        upHist_.fill(0.0f);
        downEven_.fill(0.0f);
        downOdd_.fill(0.0f);
        upPos_ = downPos_ = 0;
    }

    void up(float x, float& y0, float& y1) {
        upHist_[(size_t) upPos_] = x;
        y0 = upHist_[(size_t) ((upPos_ - kLatency + kRing) % kRing)];
        float acc = 0.0f;
        for (int k = 0; k < kOddTaps; ++k)
            acc += taps()[(size_t) k] * upHist_[(size_t) ((upPos_ - k + kRing) % kRing)];
        y1 = 2.0f * acc;
        upPos_ = (upPos_ + 1) % kRing;
    }

    float down(float x0, float x1) {
        downEven_[(size_t) downPos_] = x0;
        downOdd_[(size_t) downPos_] = x1;
        float acc = 0.5f * downEven_[(size_t) ((downPos_ - kLatency + kRing) % kRing)];
        for (int k = 0; k < kOddTaps; ++k)
            acc += taps()[(size_t) k] * downOdd_[(size_t) ((downPos_ - k - 1 + kRing) % kRing)];
        downPos_ = (downPos_ + 1) % kRing;
        return acc;
    }

private:
    static const std::array<float, kOddTaps>& taps() {
        static const std::array<float, kOddTaps> t = [] {
            std::array<float, kOddTaps> out{};
            const int n = 2 * kHalf;
            for (int k = 0; k < kOddTaps; ++k) {
                const int i = 2 * k + 1;
                const double m = (double) (i - kHalf);
                const double sinc = std::sin(kHalfPi * m) / (kPi * m);
                const double w = 0.35875 - 0.48829 * std::cos(kTwoPi * i / n)
                                 + 0.14128 * std::cos(2.0 * kTwoPi * i / n)
                                 - 0.01168 * std::cos(3.0 * kTwoPi * i / n);
                out[(size_t) k] = (float) (sinc * w);
            }
            double sum = 0.0;
            for (float v : out) sum += v;
            for (auto& v : out) v = (float) (v * 0.5 / sum);
            return out;
        }();
        return t;
    }

    std::array<float, kRing> upHist_{}, downEven_{}, downOdd_{};
    int upPos_ = 0, downPos_ = 0;
};

class Oversampler {
public:
    void prepare(int factor) {
        factor_ = factor >= 4 ? 4 : factor >= 2 ? 2 : 1;
        reset();
    }
    void reset() {
        first_.reset();
        second_.reset();
    }
    int factor() const { return factor_; }
    int latency() const {
        return factor_ == 4 ? 3 * HalfBandStage::kLatency
             : factor_ == 2 ? 2 * HalfBandStage::kLatency : 0;
    }

    template <class Shape>
    float process(float x, Shape shape) {
        if (factor_ == 1) return shape(x);
        float a0, a1;
        first_.up(x, a0, a1);
        if (factor_ == 2) {
            const float s0 = shape(a0);
            const float s1 = shape(a1);
            return first_.down(s0, s1);
        }
        float b0, b1, b2, b3;
        second_.up(a0, b0, b1);
        second_.up(a1, b2, b3);
        const float s0 = shape(b0);
        const float s1 = shape(b1);
        const float c0 = second_.down(s0, s1);
        const float s2 = shape(b2);
        const float s3 = shape(b3);
        const float c1 = second_.down(s2, s3);
        return first_.down(c0, c1);
    }

private:
    int factor_ = 1;
    HalfBandStage first_, second_;
};

}
