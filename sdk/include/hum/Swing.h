#pragma once
#include <algorithm>
#include <cmath>
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

template <class Params, class Xport>
Groove resolve(const Params& params, const Xport& transport, int ownGridTicks) {
    if (params.get("SwingFollow", 1.0) >= 0.5) return transport.groove();
    return {params.get("Swing", 0.0), ownGridTicks};
}

}
