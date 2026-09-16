// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/library/BankLibrary.h"
#include "core/library/UserLibrary.h"
#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>
#include "core/graph/GraphLayout.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PerfBox.h"
#include "core/graph/PodModel.h"
#include "core/packs/Roles.h"
#include "hum/Registry.h"
#include "io/PatchWriter.h"
#include "io/ModRouteBuild.h"
#include "io/PatchLoader.h"
#include "gui/app/AppSettings.h"

namespace hum {

std::size_t EngineHost::approxBytes(const UndoState& s) {
    auto params = [](const std::vector<Parameter>& v) {
        std::size_t n = v.capacity() * sizeof(Parameter);
        for (const auto& p : v) n += p.name.size() + p.type.size() + p.text.size();
        return n;
    };
    auto pattern = [](const Pattern& p) {
        std::size_t n = p.channels.capacity() * sizeof(PatternChannel);
        for (const auto& ch : p.channels)
            n += ch.matrix.size() + ch.name.size() + ch.audioFile.size() + ch.snap.size()
                 + ch.triggers.capacity() * sizeof(int)
                 + ch.timeSignatures.capacity() * sizeof(PatternTimeSig);
        return n;
    };
    std::size_t n = sizeof(UndoState);
    if (s.snapshot) {
        n += s.model.organisms.capacity() * sizeof(OrganismModel);
        for (const auto& c : s.model.organisms) {
            n += c.name.size() + c.classRaw.size() + c.displayClass.size() + c.kind.size();
            n += params(c.properties) + pattern(c.pattern);
            for (const auto& pr : c.presets) n += pr.name.size() + params(pr.properties);
            for (const auto& l : c.automation)
                n += l.propertyName.size()
                     + l.points.capacity() * sizeof(AutomationBreakpoint);
            n += c.midiSources.capacity() * sizeof(MidiControllerSource)
                 + c.oscSources.capacity() * sizeof(OscControllerSource)
                 + c.modSources.capacity() * sizeof(ModControllerSource);
        }
        n += s.model.connections.capacity() * sizeof(ConnectionModel) * 3;
        n += s.model.views.capacity() * sizeof(OrganismView);
        n += s.positions.size() * (sizeof(juce::Point<int>) + 32);
    } else {
        for (const auto& r : s.params) n += sizeof(ParamRevert) + r.organism.size()
                                            + r.param.size() + r.before.text.size();
        for (const auto& r : s.patterns) n += sizeof(PatternRevert) + r.organism.size()
                                             + pattern(r.before);
    }
    return n;
}

void EngineHost::trimHistory(std::deque<UndoState>& d) {
    while (d.size() > kUndoMaxSteps) d.pop_front();
    std::size_t total = 0;
    for (const auto& s : d) total += approxBytes(s);
    while (d.size() > 1 && total > kUndoMaxBytes) {
        total -= approxBytes(d.front());
        d.pop_front();
    }
}

void EngineHost::pushUndo() {
    const bool wasDirty = dirty_;
    dirty_ = true;
    ++changeStamp_;
    paramEditKey_.clear();
    paramStepOpen_ = false;
    if (inTxn_ && txnPushed_) return;
    UndoState s;
    s.snapshot = true;
    s.wasDirty = wasDirty;
    s.model = model_;
    s.positions = positions_;
    undo_.push_back(std::move(s));
    trimHistory(undo_);
    redo_.clear();
    if (inTxn_) txnPushed_ = true;
}

void EngineHost::pushParamStep() {
    bool wasDirty = dirty_;
    dirty_ = true;
    ++changeStamp_;
    if (!undo_.empty() && !undo_.back().snapshot && undo_.back().params.empty()
        && undo_.back().patterns.empty()) {
        wasDirty = undo_.back().wasDirty;
        undo_.pop_back();
    }
    UndoState s;
    s.snapshot = false;
    s.wasDirty = wasDirty;
    undo_.push_back(std::move(s));
    trimHistory(undo_);
    redo_.clear();
    paramStepOpen_ = true;
}

void EngineHost::recordParamRevert(const std::string& organism, const std::string& param) {
    if (!paramStepOpen_ || undo_.empty() || undo_.back().snapshot) return;
    auto& step = undo_.back();
    for (const auto& r : step.params)
        if (r.organism == organism && r.param == param) return;
    ParamRevert r;
    r.organism = organism;
    r.param = param;
    r.existed = false;
    if (const auto* cm = model_.byName(organism))
        for (const auto& p : cm->properties)
            if (p.name == param) { r.before = p; r.existed = true; break; }
    if (!r.existed) { r.before.name = param; }
    step.params.push_back(std::move(r));
}

void EngineHost::recordPatternRevert(const std::string& organism) {
    if (!paramStepOpen_ || undo_.empty() || undo_.back().snapshot) return;
    auto& step = undo_.back();
    for (const auto& r : step.patterns)
        if (r.organism == organism) return;
    const auto* cm = model_.byName(organism);
    if (cm == nullptr) return;
    step.patterns.push_back({organism, cm->pattern});
}

EngineHost::UndoState EngineHost::inverseOf(const UndoState& step) const {
    UndoState inv;
    inv.snapshot = false;
    for (const auto& r : step.params) {
        ParamRevert back;
        back.organism = r.organism;
        back.param = r.param;
        back.existed = false;
        if (const auto* cm = model_.byName(r.organism))
            for (const auto& p : cm->properties)
                if (p.name == r.param) { back.before = p; back.existed = true; break; }
        if (!back.existed) back.before.name = r.param;
        inv.params.push_back(std::move(back));
    }
    for (const auto& r : step.patterns)
        if (const auto* cm = model_.byName(r.organism))
            inv.patterns.push_back({r.organism, cm->pattern});
    return inv;
}

void EngineHost::applyDelta(const UndoState& step) {
    const bool wasOpen = paramStepOpen_;
    paramStepOpen_ = false;
    DerivedControlScope derived(*this);
    {
        PatternSyncBatch batch(*this);
        for (const auto& r : step.patterns)
            if (auto* cm = mutableByName(r.organism)) {
                cm->pattern = r.before;
                syncPattern(r.organism);
            }
    }
    for (auto it = step.params.rbegin(); it != step.params.rend(); ++it) {
        const auto& r = *it;
        auto* cm = mutableByName(r.organism);
        if (cm == nullptr) continue;
        if (!r.existed) {
            auto& v = cm->properties;
            v.erase(std::remove_if(v.begin(), v.end(),
                                   [&](const Parameter& p) { return p.name == r.param; }),
                    v.end());
            continue;
        }
        const std::string* curText = nullptr;
        for (const auto& p : cm->properties)
            if (p.name == r.param) { curText = &p.text; break; }
        if (curText != nullptr && *curText != r.before.text)
            setParamText(r.organism, r.param, r.before.text);
        if (r.before.isRange) setParamRange(r.organism, r.param, r.before.rangeMin, r.before.rangeMax);
        else                  setParam(r.organism, r.param, r.before.value);
    }
    paramStepOpen_ = wasOpen;
}

void EngineHost::beginTransaction() {
    if (txnDepth_++ == 0) { inTxn_ = true; txnPushed_ = false; }
}

void EngineHost::endTransaction() {
    if (txnDepth_ > 0 && --txnDepth_ > 0) return;
    txnDepth_ = 0;
    inTxn_ = false;
    txnPushed_ = false;
    if (rebuildDue_) rebuild();
}

bool EngineHost::undo() {
    if (undo_.empty()) return false;
    const bool wasDirty = dirty_;
    paramEditKey_.clear();
    paramStepOpen_ = false;
    auto s = std::move(undo_.back());
    undo_.pop_back();
    if (!s.snapshot) {
        auto inv = inverseOf(s);
        inv.wasDirty = wasDirty;
        redo_.push_back(std::move(inv));
    trimHistory(redo_);
        applyDelta(s);
        dirty_ = s.wasDirty;
        return true;
    }
    UndoState cur;
    cur.snapshot = true;
    cur.wasDirty = wasDirty;
    cur.model = model_;
    cur.positions = positions_;
    redo_.push_back(std::move(cur));
    trimHistory(redo_);
    model_ = std::move(s.model);
    positions_ = std::move(s.positions);
    modelSwapped_ = true;
    requestRebuild();
    dirty_ = s.wasDirty;
    return true;
}

bool EngineHost::redo() {
    if (redo_.empty()) return false;
    const bool wasDirty = dirty_;
    paramEditKey_.clear();
    paramStepOpen_ = false;
    auto s = std::move(redo_.back());
    redo_.pop_back();
    if (!s.snapshot) {
        auto inv = inverseOf(s);
        inv.wasDirty = wasDirty;
        undo_.push_back(std::move(inv));
        applyDelta(s);
        dirty_ = s.wasDirty;
        return true;
    }
    UndoState cur;
    cur.snapshot = true;
    cur.wasDirty = wasDirty;
    cur.model = model_;
    cur.positions = positions_;
    undo_.push_back(std::move(cur));
    model_ = std::move(s.model);
    positions_ = std::move(s.positions);
    modelSwapped_ = true;
    requestRebuild();
    dirty_ = s.wasDirty;
    return true;
}

}
