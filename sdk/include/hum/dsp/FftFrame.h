// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {

class FftFrame {
public:
    void prepare(int size) {
        size_ = 1;
        while (size_ < size) size_ <<= 1;
        re_.assign((size_t) size_, 0.0f);
        im_.assign((size_t) size_, 0.0f);
        mag_.assign((size_t) (size_ / 2 + 1), 0.0f);
        window_.resize((size_t) size_);
        for (int i = 0; i < size_; ++i)
            window_[(size_t) i] =
                0.5f * (1.0f - std::cos(2.0f * kPiF * (float) i / (float) (size_ - 1)));
        reverse_.resize((size_t) size_);
        for (int i = 0; i < size_; ++i) {
            int rev = 0;
            for (int bit = 1; bit < size_; bit <<= 1) {
                rev <<= 1;
                if (i & bit) rev |= 1;
            }
            reverse_[(size_t) i] = rev;
        }
    }

    int size() const { return size_; }
    int bins() const { return size_ / 2 + 1; }
    const std::vector<float>& magnitudes() const { return mag_; }
    std::vector<float>& magnitudes() { return mag_; }

    double binHz(double sampleRate) const { return size_ > 0 ? sampleRate / size_ : 0.0; }

    void measure(const float* samples) {
        if (size_ <= 0) return;
        for (int i = 0; i < size_; ++i) {
            re_[(size_t) reverse_[(size_t) i]] = samples[i] * window_[(size_t) i];
            im_[(size_t) reverse_[(size_t) i]] = 0.0f;
        }
        transform();
        const float scale = 4.0f / (float) size_;
        for (int k = 0; k < bins(); ++k) {
            const float a = re_[(size_t) k], b = im_[(size_t) k];
            mag_[(size_t) k] = std::sqrt(a * a + b * b) * scale;
        }
    }

    float magnitudeAt(double bin) const {
        if (mag_.empty() || bin < 0.0) return 0.0f;
        const int k = (int) bin;
        if (k + 1 >= (int) mag_.size()) return k < (int) mag_.size() ? mag_[(size_t) k] : 0.0f;
        const float frac = (float) (bin - k);
        return mag_[(size_t) k] * (1.0f - frac) + mag_[(size_t) (k + 1)] * frac;
    }

    float peakNear(double bin, int reach) const {
        float best = 0.0f;
        const int lo = (int) bin - reach, hi = (int) bin + reach + 1;
        for (int k = lo; k <= hi; ++k)
            if (k >= 0 && k < (int) mag_.size()) best = std::max(best, mag_[(size_t) k]);
        return best;
    }

private:
    void transform() {
        for (int span = 2; span <= size_; span <<= 1) {
            const double step = -2.0 * kPi / span;
            const int half = span / 2;
            for (int start = 0; start < size_; start += span)
                for (int i = 0; i < half; ++i) {
                    const double angle = step * i;
                    const auto wr = (float) std::cos(angle);
                    const auto wi = (float) std::sin(angle);
                    const size_t a = (size_t) (start + i), b = (size_t) (start + i + half);
                    const float tr = re_[b] * wr - im_[b] * wi;
                    const float ti = re_[b] * wi + im_[b] * wr;
                    re_[b] = re_[a] - tr;
                    im_[b] = im_[a] - ti;
                    re_[a] += tr;
                    im_[a] += ti;
                }
        }
    }

    int size_ = 0;
    std::vector<float> re_, im_, mag_, window_;
    std::vector<int> reverse_;
};

}
