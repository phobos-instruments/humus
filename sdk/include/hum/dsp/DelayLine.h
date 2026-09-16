// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>
#include <cmath>

namespace hum {

class DelayLine {
public:
    enum class Interp { Linear, Cubic };

    void prepare(int maxSamples) {
        buf_.assign((size_t) std::max(4, maxSamples), 0.0f);
        write_ = 0;
    }
    void clear() { std::fill(buf_.begin(), buf_.end(), 0.0f); write_ = 0; }
    void setInterp(Interp m) { interp_ = m; }

    void write(float x) {
        buf_[(size_t) write_] = x;
        if (++write_ >= (int) buf_.size()) write_ = 0;
    }

    float read(float delay) const {
        const int n = (int) buf_.size();
        const float lo = interp_ == Interp::Cubic ? 1.0f : 0.0f;
        const float hi = (float) (n - (interp_ == Interp::Cubic ? 3 : 1));
        const float d = std::min(std::max(delay, lo), std::max(lo, hi));
        float rp = (float) write_ - d;
        while (rp < 0) rp += n;
        const int i0 = (int) rp;
        const float f = rp - (float) i0;
        auto at = [&](int k) { int i = i0 + k; i %= n; if (i < 0) i += n; return buf_[(size_t) i]; };

        if (interp_ == Interp::Linear)
            return at(0) + f * (at(1) - at(0));

        const float xm = at(-1), x0 = at(0), x1 = at(1), x2 = at(2);
        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm);
        const float c2 = xm - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + c0;
    }

private:
    std::vector<float> buf_;
    int write_ = 0;
    Interp interp_ = Interp::Linear;
};

}
