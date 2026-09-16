// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {

inline int stepPlayhead(bool playing, double beats, const std::string& resolution, int n) {
    if (!playing || n <= 0) return -1;
    const double stepsPerBeat = (double) Pattern::kTicksPerBeat / (double) stepTicksFor(resolution);
    const long abs = (long) std::floor(beats * stepsPerBeat + 1e-9);
    return abs < 0 ? -1 : (int) (abs % n);
}

}
