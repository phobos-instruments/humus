// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstddef>

namespace hum {

class RateRing {
public:
    static constexpr int kChunk = 64;

    void reset() {
        l_.fill(0.0f);
        r_.fill(0.0f);
        write_ = 0;
        read_ = 0.0;
    }

    void push(float l, float r) {
        l_[(size_t) (write_ % kRing)] = l;
        r_[(size_t) (write_ % kRing)] = r;
        ++write_;
    }

    bool needsMore(int numSamples, double ratio) const {
        return (double) write_ < read_ + (double) numSamples * ratio + 2.0;
    }

    void read(float* L, float* R, int numSamples, double ratio, float level) {
        for (int i = 0; i < numSamples; ++i) {
            const auto idx = (long long) read_;
            const float frac = (float) (read_ - (double) idx);
            const auto i0 = (size_t) (idx % kRing);
            const auto i1 = (size_t) ((idx + 1) % kRing);
            const float l = (l_[i0] + (l_[i1] - l_[i0]) * frac) * level;
            const float r = (r_[i0] + (r_[i1] - r_[i0]) * frac) * level;
            if (R != L) { L[i] = l; R[i] = r; }
            else        { L[i] = (l + r) * 0.5f; }
            read_ += ratio;
        }
        if (read_ > 1.0e9) {
            const auto whole = (long long) read_ - (long long) read_ % kRing;
            read_ -= (double) whole;
            write_ -= (int) whole;
        }
    }

private:
    static constexpr int kRing = 4096;
    std::array<float, kRing> l_{}, r_{};
    int write_ = 0;
    double read_ = 0.0;
};

}
