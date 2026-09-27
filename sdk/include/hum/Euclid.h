// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>

namespace hum::euclid {

inline bool hit(long long step, int steps, int hits, int rotate) {
    if (steps < 1) return false;
    const long long spread = std::clamp(hits, 0, steps);
    const long long at = (((step - rotate) % steps) + steps) % steps;
    return (at * spread) % steps < spread;
}

}
