// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hum/dsp/Biquad.h"
#include "hum/dsp/DspMath.h"
#include "hum/dsp/Interpolation.h"

namespace hum::lofi {

constexpr int kAntiAliasPoles = 8;
constexpr double kAntiAliasCutoff = 0.42;

struct Dither {
    std::uint32_t state = 0x9e3779b9u;

    explicit Dither(std::uint32_t seed = 0) : state(0x9e3779b9u + seed * 0x6d2b79f5u) {
        if (state == 0) state = 0x9e3779b9u;
    }

    float next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float) (state >> 8) * (1.0f / 16777216.0f);
    }
};

inline int decimatedFrames(int frames, double srcRate, double dstRate) {
    if (frames < 2 || dstRate <= 0.0 || srcRate <= dstRate) return frames;
    return std::max(2, (int) ((double) frames * dstRate / srcRate));
}

inline void antiAlias(float* x, int frames, double srcRate, double dstRate) {
    if (x == nullptr || frames < 2 || dstRate <= 0.0 || srcRate <= dstRate) return;
    for (int stage = 0; stage < kAntiAliasPoles / 2; ++stage) {
        const double angle = (2.0 * stage + 1.0) * kPi / (2.0 * kAntiAliasPoles);
        Biquad pole;
        pole.setLowpass(srcRate, dstRate * kAntiAliasCutoff, 1.0 / (2.0 * std::cos(angle)));
        for (int i = 0; i < frames; ++i) x[i] = pole.process(x[i]);
    }
}

inline void decimate(const float* src, int frames, float* dst, int outFrames, double step) {
    if (src == nullptr || dst == nullptr || step <= 0.0) return;
    for (int i = 0; i < outFrames; ++i)
        dst[i] = sampleAt(src, frames, (double) i * step, Interp::Sinc);
}

inline void quantise(float* x, int frames, int bits, Dither& dither) {
    if (x == nullptr || bits >= 24) return;
    const float steps = (float) (1 << (std::clamp(bits, 2, 24) - 1));
    for (int i = 0; i < frames; ++i) {
        if (x[i] == 0.0f) continue;
        const float triangular = dither.next() + dither.next() - 1.0f;
        const float held = std::clamp(x[i], -1.0f, 1.0f);
        x[i] = std::clamp(std::round(held * steps + triangular) / steps, -1.0f, 1.0f);
    }
}

}
