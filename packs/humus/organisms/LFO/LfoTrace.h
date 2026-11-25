// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <atomic>

#include "hum/caps/Graph.h"

namespace hum {

class LfoTrace {
public:
    static constexpr int kPoints = MotionTrace::kPoints;

    void reset() {
        count_ = 0;
        low_ = high_ = 0.0;
        written_.store(0, std::memory_order_release);
    }
    void setSlice(int samples) { slice_ = std::max(1, samples); }

    void add(double v) {
        if (count_ == 0) low_ = high_ = v;
        low_ = std::min(low_, v);
        high_ = std::max(high_, v);
        if (++count_ >= slice_) flush();
    }

    int read(MotionTrace::Point* out, int capacity) const {
        const unsigned written = written_.load(std::memory_order_acquire);
        const int n = std::min({capacity, kPoints, (int) std::min<unsigned>(written, (unsigned) kPoints)});
        for (int i = 0; i < n; ++i) {
            const size_t at = (size_t) ((written - (unsigned) n + (unsigned) i) % (unsigned) kPoints);
            out[i] = {lows_[at].load(std::memory_order_relaxed), highs_[at].load(std::memory_order_relaxed)};
        }
        return n;
    }

private:
    void flush() {
        const unsigned written = written_.load(std::memory_order_relaxed);
        const size_t at = (size_t) (written % (unsigned) kPoints);
        lows_[at].store((float) low_, std::memory_order_relaxed);
        highs_[at].store((float) high_, std::memory_order_relaxed);
        written_.store(written + 1, std::memory_order_release);
        count_ = 0;
    }

    std::array<std::atomic<float>, kPoints> lows_{}, highs_{};
    std::atomic<unsigned> written_{0};
    int count_ = 0, slice_ = 1;
    double low_ = 0.0, high_ = 0.0;
};

}
