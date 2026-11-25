// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>

#include "hum/Euclid.h"

namespace hum::ringmute {

inline constexpr int kMaskSteps = 32;
inline constexpr double kWholeMask = 4294967295.0;

inline std::uint32_t maskFrom(double value) {
    return (std::uint32_t) std::llround(value < 0.0 ? 0.0 : value > kWholeMask ? kWholeMask : value);
}

inline double valueFrom(std::uint32_t mask) { return (double) mask; }

inline std::uint32_t bitAt(long long step, int steps, int rotate) {
    if (steps < 1) return 0u;
    const long long at = (((step - rotate) % steps) + steps) % steps;
    return at < kMaskSteps ? std::uint32_t{1} << at : 0u;
}

inline bool plays(long long step, int steps, int hits, int rotate, std::uint32_t muted) {
    return euclid::hit(step, steps, hits, rotate) && (muted & bitAt(step, steps, rotate)) == 0;
}

}
