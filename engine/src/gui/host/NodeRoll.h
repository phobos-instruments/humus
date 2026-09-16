// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include "gui/editor/BankSlotSpec.h"
#include "gui/host/EngineHost.h"
#include "gui/host/NodeRandomize.h"

namespace hum {

inline void randomizeNode(EngineHost& host, const std::string& name, bool undoable = true) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return;
    auto& r = juce::Random::getSystemRandom();
    if (undoable) host.pushParamStep();
    EngineHost::NoLatchScope noLatch(host);
    EngineHost::PatternSyncBatch batch(host);
    auto* live = dynamic_cast<LiveParamRange*>(host.liveOrganism(name));
    for (auto [param, value] : randomParamValues(schemaFor(cm->classRaw), r)) {
        if (cm->rollLocked.count(param) > 0) continue;
        double lo = 0.0, hi = 0.0;
        if (live != nullptr && live->liveParamRange(param, lo, hi) && hi > lo)
            value = std::round(lo + r.nextDouble() * (hi - lo));
        host.setParam(name, param, value);
    }

    if (auto* live = dynamic_cast<LiveParamRange*>(host.liveOrganism(name));
        live != nullptr && dynamic_cast<PatchBank*>(host.liveOrganism(name)) != nullptr) {
        const auto choices = banks::rollable(bankSlotFor(cm->displayClass), cm->displayClass);
        if (!choices.empty() && cm->rollLocked.count("File") == 0) {
            host.setParamText(name, "File", choices[(size_t) r.nextInt((int) choices.size())]);
            double lo = 0.0, hi = 0.0;
            if (cm->rollLocked.count("Patch") == 0
                && live->liveParamRange("Patch", lo, hi) && hi >= lo)
                host.setParam(name, "Patch",
                              std::floor(lo + r.nextDouble() * (hi - lo + 1.0)));
        }
    }

    if (auto* rl = dynamic_cast<RollListener*>(host.liveOrganism(name))) rl->rolled();

    for (const auto& d : schemaFor(cm->classRaw))
        if (!d.randomText.empty() && cm->rollLocked.count(d.name) == 0)
            host.setParamText(name, d.name, rolledText(d.randomText, r));
    rollPattern(host, name, patternRollOf(*cm), r);
}

}
