// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <string_view>

#include "core/packs/PackRegistry.h"
#include "core/params/Randomize.h"
#include "core/params/RollScope.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostPattern.h"
#include "hum/caps/Params.h"
#include "hum/Organism.h"
#include "io/PatchDocument.h"

namespace hum {

inline std::string patternRollOf(const OrganismModel& cm) {
    const auto* m = PackRegistry::instance().classManifest(cm.displayClass);
    return m != nullptr ? m->roll : std::string();
}

inline void rollPattern(BrickHost& host, const std::string& name, std::string_view kind,
                        juce::Random& r) {
    auto& patterns = host.patterns();
    JuceDice dice(r);
    if (kind == roll::kBassline) {
        patterns.ensureMatrix(name, "bassline-pattern-matrix", 16);
        const auto cur = patterns.basslineSteps(name);
        const auto rolled = randomBassline((int) cur.size(), basslineRoot(cur), dice);
        for (int i = 0; i < (int) rolled.size(); ++i) patterns.setBasslineStep(name, i, rolled[(size_t) i]);
    } else if (kind == roll::kTriggerSteps) {
        patterns.ensureMatrix(name, "trigger-tie-matrix", 16);
        const auto cur = patterns.arpSteps(name);
        const auto cells = randomTriggerRow((int) cur.size(), 0.35 + r.nextDouble() * 0.35, dice);
        for (int i = 0; i < (int) cells.size(); ++i) {
            ArpStep s;
            s.trigger = cells[(size_t) i];
            s.tie = r.nextDouble() < 0.12;
            patterns.setArpStep(name, i, s);
        }
    } else if (kind == roll::kDrumRows) {
        patterns.ensureBanks(name, 8, kPatternBanks);
        for (int row = 0; row < 8; ++row) {
            patterns.clearChannel(name, row);
            const auto cells = randomTriggerRow(16, 0.10 + r.nextDouble() * 0.35, dice);
            for (int s = 0; s < 16; ++s)
                if (cells[(size_t) s]) patterns.addTrigger(name, row, s * (Pattern::kTicksPerBeat / 4));
        }
    }
}

inline bool nodeSupportsRandom(BrickHost& host, const std::string& name) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    if (!patternRollOf(*cm).empty()) return true;
    if (dynamic_cast<RollListener*>(host.liveOrganism(name)) != nullptr) return true;
    for (const auto& d : schemaFor(cm->classRaw))
        if (!d.randomText.empty()) return true;
    return hasRandomParams(schemaFor(cm->classRaw));
}

inline bool paramSupportsRandom(BrickHost& host, const std::string& name,
                                const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    if (cm->rollLocked.count(param) > 0) return false;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return !d.isText && d.max > d.min;
    return false;
}

inline bool rollTouchesParam(BrickHost& host, const std::string& name,
                             const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    if ((param == "File" || param == "Patch")
        && dynamic_cast<PatchBank*>(host.liveOrganism(name)) != nullptr)
        return true;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return !d.randomText.empty() || (d.randomize && !d.isText);
    return false;
}

inline void randomizeParam(BrickHost& host, const std::string& name,
                           const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return;
    for (const auto& d : schemaFor(cm->classRaw)) {
        if (d.name != param) continue;
        if (d.isText || d.max <= d.min) return;
        auto& r = juce::Random::getSystemRandom();
        double lo = d.rollLow(), hi = d.rollHigh();
        double a = 0.0, b = 0.0;
        if (auto* live = dynamic_cast<LiveParamRange*>(host.liveOrganism(name)))
            if (live->liveParamRange(param, a, b) && b > a) { lo = a; hi = b; }
        if (d.isRange) {
            const auto span = rollSpan(d, lo, hi, r);
            host.pushParamStep();
            host.setParamRange(name, param, span.low, span.high);
            host.noteNodeRolled(name);
            return;
        }
        double v = lo + r.nextDouble() * (hi - lo);
        if (d.isBool || d.isEnum || d.isInt) v = std::round(v);
        host.pushParamStep();
        host.setParam(name, param, juce::jlimit(lo, hi, v));
        return;
    }
}

inline bool paramHasDefault(BrickHost& host, const std::string& name,
                            const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return !d.isText;
    return false;
}

inline void resetParam(BrickHost& host, const std::string& name, const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return;
    for (const auto& d : schemaFor(cm->classRaw)) {
        if (d.name != param) continue;
        if (d.isText) return;
        host.pushParamStep();
        if (d.isRange) host.setParamRange(name, param, d.def, d.defMax);
        else host.setParam(name, param, d.def);
        host.noteNodeRolled(name);
        return;
    }
}

inline std::vector<rollscope::Candidate> rollCandidates(BrickHost& host) {
    std::vector<rollscope::Candidate> out;
    for (const auto& c : host.model().organisms)
        out.push_back({c.name, nodeSupportsRandom(host, c.name), rollscope::isProtected(c.rollLocked)});
    return out;
}

inline void protectPod(BrickHost& host, const std::string& pod, bool on) {
    host.beginUndoGroup();
    for (const auto& n : rollscope::members(rollCandidates(host), pod))
        host.setRollLocked(n, rollscope::kWholeOrganism, on);
    host.endUndoGroup();
}

}
