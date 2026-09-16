// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <string>

#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum::swing {

struct Groove {
    double amount = 0.0;
    int gridTicks = Pattern::kTicksPerBeat / 4;
};

inline double cellFraction(double amount) {
    return std::clamp(amount, 0.0, 1.0) / 3.0;
}

inline double delayTicks(double tick, const Groove& g) {
    if (g.amount <= 0.0 || g.gridTicks <= 0) return 0.0;
    const long cell = (long) std::floor(tick / (double) g.gridTicks);
    return (cell & 1) != 0 ? cellFraction(g.amount) * (double) g.gridTicks : 0.0;
}

inline double delaySteps(double step, double ticksPerStep, const Groove& g) {
    if (ticksPerStep <= 0.0) return 0.0;
    return delayTicks(step * ticksPerStep, g) / ticksPerStep;
}

inline Groove grooveFor(double amount, const std::string& unit) {
    return {amount, stepTicksFor(unit)};
}

inline constexpr int kGridDenominators[] = {8, 16, 32};
inline constexpr int kGridChoices = (int) std::size(kGridDenominators);

inline constexpr bool gridsDivideTheBar() {
    for (int d : kGridDenominators)
        if ((Pattern::kTicksPerBeat * 4) % d != 0) return false;
    return true;
}
static_assert(gridsDivideTheBar());

inline int gridIndexClamped(long index) {
    return (int) std::clamp(index, 0L, (long) kGridChoices - 1);
}

inline int gridTicksAt(long index) {
    return (Pattern::kTicksPerBeat * 4) / kGridDenominators[gridIndexClamped(index)];
}

inline std::string gridUnitAt(long index) {
    return "1/" + std::to_string(kGridDenominators[gridIndexClamped(index)]);
}

inline int gridIndexOf(int gridTicks) {
    int best = 0;
    for (int i = 1; i < kGridChoices; ++i)
        if (std::abs(gridTicksAt(i) - gridTicks) < std::abs(gridTicksAt(best) - gridTicks)) best = i;
    return best;
}

template <class Params, class Xport>
Groove resolve(const Params& params, const Xport& transport, int ownGridTicks) {
    if (params.get("SwingFollow", 1.0) >= 0.5) return transport.groove();
    return {params.get("Swing", 0.0), ownGridTicks};
}

}
