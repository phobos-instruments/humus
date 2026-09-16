// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {

enum class Interp { Linear, Cubic, Sinc };

struct SincTable {
    static constexpr int kTaps = 8;
    static constexpr int kPhases = 512;
    std::vector<float> w;

    SincTable() {
        w.resize((size_t) kPhases * kTaps);
        const double pi = kPi;
        const double L = kTaps / 2.0;
        for (int p = 0; p < kPhases; ++p) {
            const double frac = (double) p / kPhases;
            double sum = 0.0;
            for (int t = 0; t < kTaps; ++t) {
                const double x = (double) (t - kTaps / 2 + 1) - frac;
                const double s = (std::abs(x) < 1e-9) ? 1.0 : std::sin(pi * x) / (pi * x);
                double wn = 0.0;
                if (std::abs(x) < L)
                    wn = 0.42 + 0.5 * std::cos(pi * x / L) + 0.08 * std::cos(2.0 * pi * x / L);
                const double v = s * wn;
                w[(size_t) p * kTaps + t] = (float) v;
                sum += v;
            }
            if (sum > 1e-9)
                for (int t = 0; t < kTaps; ++t) w[(size_t) p * kTaps + t] /= (float) sum;
        }
    }
};

inline const SincTable& sincTable() { static const SincTable t; return t; }

inline float sampleAt(const float* x, int64_t len, double pos, Interp mode) {
    if (len <= 0) return 0.0f;
    auto at = [&](int64_t i) -> float {
        if (i < 0) i = 0; else if (i >= len) i = len - 1;
        return x[i];
    };
    const int64_t i0 = (int64_t) std::floor(pos);
    const float f = (float) (pos - (double) i0);

    switch (mode) {
        case Interp::Linear:
            return at(i0) + (at(i0 + 1) - at(i0)) * f;

        case Interp::Cubic: {
            const float xm1 = at(i0 - 1), x0 = at(i0), x1 = at(i0 + 1), x2 = at(i0 + 2);
            const float a = -0.5f * xm1 + 1.5f * x0 - 1.5f * x1 + 0.5f * x2;
            const float b = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
            const float c = -0.5f * xm1 + 0.5f * x1;
            return ((a * f + b) * f + c) * f + x0;
        }

        case Interp::Sinc: {
            const auto& tbl = sincTable();
            int p = (int) (f * SincTable::kPhases);
            if (p < 0) p = 0; else if (p >= SincTable::kPhases) p = SincTable::kPhases - 1;
            const float* k = &tbl.w[(size_t) p * SincTable::kTaps];
            float acc = 0.0f;
            for (int t = 0; t < SincTable::kTaps; ++t)
                acc += k[t] * at(i0 - SincTable::kTaps / 2 + 1 + t);
            return acc;
        }
    }
    return at(i0);
}

}
