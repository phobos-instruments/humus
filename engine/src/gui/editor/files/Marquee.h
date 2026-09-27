// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

namespace hum::files {

inline constexpr double kMarqueeRestMs = 700.0;
inline constexpr double kMarqueeEndRestMs = 1200.0;
inline constexpr double kMarqueePxPerSecond = 45.0;
inline constexpr double kMsPerSecond = 1000.0;

inline bool marqueeNeeded(double textW, double roomW) { return textW > roomW + 0.5; }

inline double marqueeOffset(double textW, double roomW, double hoveredMs) {
    if (!marqueeNeeded(textW, roomW) || hoveredMs <= 0.0) return 0.0;
    const double travel = textW - roomW;
    const double travelMs = travel / kMarqueePxPerSecond * kMsPerSecond;
    const double cycleMs = kMarqueeRestMs + travelMs + kMarqueeEndRestMs;
    const double at = std::fmod(hoveredMs, cycleMs);
    if (at <= kMarqueeRestMs) return 0.0;
    return std::min(travel, (at - kMarqueeRestMs) * kMarqueePxPerSecond / kMsPerSecond);
}

}
