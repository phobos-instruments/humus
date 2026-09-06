#pragma once
#include <cmath>
#include <string>

#include "core/Randomize.h"
#include "gui/BankSlotSpec.h"
#include "gui/EngineHost.h"
#include "hum/dsp/WaveTable.h"

namespace hum {

inline bool nodeSupportsRandom(EngineHost& host, const std::string& name) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    if (cm->displayClass == "Riff" || cm->displayClass == "Steps"
        || cm->displayClass == "Sequence")
        return true;
    return hasRandomParams(schemaFor(cm->classRaw));
}

inline bool paramSupportsRandom(EngineHost& host, const std::string& name,
                                const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    if (cm->rollLocked.count(param) > 0) return false;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return !d.isText && !d.isRange && d.max > d.min;
    return false;
}

inline bool rollTouchesParam(EngineHost& host, const std::string& name,
                             const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    if ((param == "File" || param == "Patch")
        && dynamic_cast<PatchBank*>(host.liveOrganism(name)) != nullptr)
        return true;
    if (param == "Expression" && cm->displayClass == "Math") return true;
    if (param == "Table" && cm->displayClass == "Wave") return true;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return d.randomize && !d.isText && !d.isRange;
    return false;
}

inline void randomizeParam(EngineHost& host, const std::string& name,
                           const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return;
    for (const auto& d : schemaFor(cm->classRaw)) {
        if (d.name != param) continue;
        if (d.isText || d.isRange || d.max <= d.min) return;
        auto& r = juce::Random::getSystemRandom();
        double lo = d.min, hi = d.max;
        double a = 0.0, b = 0.0;
        if (auto* live = dynamic_cast<LiveParamRange*>(host.liveOrganism(name)))
            if (live->liveParamRange(param, a, b) && b > a) { lo = a; hi = b; }
        double v = lo + r.nextDouble() * (hi - lo);
        if (d.isBool || d.isEnum || d.isInt) v = std::round(v);
        host.pushParamStep();
        host.setParam(name, param, juce::jlimit(lo, hi, v));
        return;
    }
}

inline bool paramHasDefault(EngineHost& host, const std::string& name,
                            const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return false;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return !d.isText;
    return false;
}

inline void resetParam(EngineHost& host, const std::string& name, const std::string& param) {
    const auto* cm = host.model().byName(name);
    if (cm == nullptr) return;
    for (const auto& d : schemaFor(cm->classRaw)) {
        if (d.name != param) continue;
        if (d.isText) return;
        host.pushParamStep();
        if (d.isRange) host.setParamRange(name, param, d.def, d.defMax);
        else host.setParam(name, param, d.def);
        if (host.onNodeRolled) host.onNodeRolled(name);
        return;
    }
}

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

    if (cm->displayClass == "Riff") {
        host.patterns().ensureMatrix(name, "bassline-pattern-matrix", 16);
        const auto cur = host.patterns().basslineSteps(name);
        const auto rolled = randomBassline((int) cur.size(), basslineRoot(cur), r);
        for (int i = 0; i < (int) rolled.size(); ++i)
            host.patterns().setBasslineStep(name, i, rolled[(size_t) i]);
    } else if (cm->displayClass == "Steps") {
        host.patterns().ensureMatrix(name, "trigger-tie-matrix", 16);
        const auto cur = host.patterns().arpSteps(name);
        const auto cells = randomTriggerRow((int) cur.size(),
                                            0.35 + r.nextDouble() * 0.35, r);
        for (int i = 0; i < (int) cells.size(); ++i) {
            ArpStep s;
            s.trigger = cells[(size_t) i];
            s.tie = r.nextDouble() < 0.12;
            host.patterns().setArpStep(name, i, s);
        }
    } else if (cm->displayClass == "Math" && cm->rollLocked.count("Expression") == 0) {
        host.setParamText(name, "Expression", randomFormula(r));
    } else if (cm->displayClass == "Wave" && cm->rollLocked.count("Table") == 0) {
        float t[kWaveTableLen];
        waveTableRandom((uint32_t) r.nextInt(), t, kWaveTableLen);
        host.setParamText(name, "Table", encodeWaveTable(t, kWaveTableLen));
    } else if (cm->displayClass == "Sequence") {
        host.patterns().ensureBanks(name, 8, kPatternBanks);
        for (int row = 0; row < 8; ++row) {
            host.patterns().clearChannel(name, row);
            const auto cells = randomTriggerRow(16, 0.10 + r.nextDouble() * 0.35, r);
            for (int s = 0; s < 16; ++s)
                if (cells[(size_t) s])
                    host.patterns().addTrigger(name, row, s * (Pattern::kTicksPerBeat / 4));
        }
    }
}

}
