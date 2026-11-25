// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/params/ParamSchema.h"
#include "core/params/Randomize.h"
#include "gui/host/BrickHost.h"

namespace hum {

inline constexpr double kEvolveReach = 0.08;

inline std::vector<const ParamDesc*> evolvableParams(const std::vector<ParamDesc>& schema) {
    const bool curated = hasRandomParams(schema);
    std::vector<const ParamDesc*> out;
    for (const auto& d : schema)
        if (!d.isText && !d.isRange && (!curated || d.randomize)) out.push_back(&d);
    return out;
}

inline bool nodeSupportsEvolve(BrickHost& host, const std::string& node) {
    const auto* cm = host.model().byName(node);
    return cm != nullptr && !evolvableParams(schemaFor(cm->classRaw)).empty();
}

inline void evolveNode(BrickHost& host, const std::string& node, bool undoable = true) {
    const auto* cm = host.model().byName(node);
    if (cm == nullptr) return;
    auto& r = juce::Random::getSystemRandom();
    std::vector<std::pair<std::string, double>> vals;
    for (const auto* d : evolvableParams(schemaFor(cm->classRaw))) {
        if (cm->rollLocked.count(d->name) > 0) continue;
        const double now = host.liveParamValue(node, d->name);
        double v = now + (r.nextDouble() * 2.0 - 1.0) * kEvolveReach * (d->max - d->min);
        v = juce::jlimit(std::min(d->rollLow(), now), std::max(d->rollHigh(), now), v);
        if (d->isBool || d->isEnum || d->isInt) v = std::round(v);
        vals.emplace_back(d->name, v);
    }
    if (vals.empty()) return;
    if (undoable) host.pushUndo();
    for (const auto& [param, value] : vals) host.setParam(node, param, value);
}

}
