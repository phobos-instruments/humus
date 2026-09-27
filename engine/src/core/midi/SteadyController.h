// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdlib>

#include "hum/dsp/DspMath.h"

namespace hum::steadycc {

inline constexpr int kUnseen = -1;
inline constexpr int kLowest = 0;
inline constexpr int kHighest = kMidiMax;

struct Steady {
    int applied = kUnseen;
    int heading = 0;
};

inline bool moved(Steady& s, int value) {
    if (value < 0 || value == s.applied) return false;
    const int step = value - s.applied;
    const int heading = step > 0 ? 1 : -1;
    const bool wobble = s.applied != kUnseen && std::abs(step) == 1 && heading != s.heading
                        && value != kLowest && value != kHighest;
    if (wobble) return false;
    s.heading = s.applied == kUnseen ? 0 : heading;
    s.applied = value;
    return true;
}

}
