// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

inline double steppedValue(double current, double delta, double min, double max,
                           bool logScale, bool discrete) {
    const double lo = std::min(min, max);
    const double hi = std::max(min, max);
    if (discrete) {
        const double unit = delta * (max - min);
        const double move = std::abs(unit) < 1.0 ? (delta < 0.0 ? -1.0 : 1.0) : std::round(unit);
        return std::clamp(std::round(current) + move, lo, hi);
    }
    if (logScale && min > 0.0 && max > 0.0 && current > 0.0) {
        const double t = std::log(current / min) / std::log(max / min) + delta;
        return std::clamp(min * std::pow(max / min, std::clamp(t, 0.0, 1.0)), lo, hi);
    }
    return std::clamp(current + delta * (max - min), lo, hi);
}

}
