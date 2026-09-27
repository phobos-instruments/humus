// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

namespace hum::loopfade {

constexpr double kFadeMs = 12.0;

inline int fadeLength(int loopStart, int loopEnd, double sampleRate, double ms = kFadeMs) {
    if (loopEnd <= loopStart || sampleRate <= 0.0) return 0;
    const int wanted = (int) (ms * 0.001 * sampleRate);
    const int fits = std::min(wanted, (loopEnd - loopStart) / 2);
    return std::max(0, std::min(fits, loopStart));
}

inline void fadeGains(double through, float& held, float& coming) {
    const double t = std::clamp(through, 0.0, 1.0);
    held = (float) std::sqrt(1.0 - t);
    coming = (float) std::sqrt(t);
}

}
