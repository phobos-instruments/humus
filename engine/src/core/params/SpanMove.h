// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>

#include "core/params/RangeEnd.h"

namespace hum {

inline void slideInto(double floorAt, double ceilingAt, double& lo, double& hi) {
    if (ceilingAt <= floorAt) return;
    const double width = hi - lo;
    if (lo < floorAt)   { lo = floorAt;   hi = floorAt + width; }
    if (hi > ceilingAt) { hi = ceilingAt; lo = ceilingAt - width; }
    lo = std::max(lo, floorAt);
    hi = std::min(hi, ceilingAt);
}

inline void moveSpan(RangeEnd end, double at, double floorAt, double ceilingAt,
                     double& lo, double& hi) {
    if (end == RangeEnd::Low)  { lo = std::min(at, hi); return; }
    if (end == RangeEnd::High) { hi = std::max(at, lo); return; }
    const bool opening = end == RangeEnd::Spread;
    const double centre = opening ? 0.5 * (lo + hi) : at;
    const double half = 0.5 * std::max(0.0, opening ? at : hi - lo);
    lo = centre - half;
    hi = centre + half;
    slideInto(floorAt, ceilingAt, lo, hi);
}

}
