// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "io/ModRouteBuild.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "core/params/ParamSchema.h"
#include "core/graph/PerfBox.h"
#include "gui/editor/ControlDefaults.h"
#include "core/graph/RtWord.h"

#include "gui/host/AutomationLanes.h"

namespace hum {

using namespace lanes;

void EngineHost::pinPerformanceControls() {
    if (!playing_) return;
    const double beat = captureStartBeat_;
    std::set<std::pair<std::string, std::string>> targets;
    for (const auto& e : midiState_.map.entries())      targets.insert({e.organism, e.param});
    for (const auto& e : osc().map().entries())   targets.insert({e.organism, e.param});
    for (const auto& e : mod().map().entries())   targets.insert({e.organism, e.param});
    if (!model_.metapad.snapshots.empty())
        if (const auto meta = metapadNodeNameIfAny(); !meta.empty()) {
            targets.insert({meta, kMetaXParam});
            targets.insert({meta, kMetaYParam});
        }
    bool any = false;
    for (const auto& [node, param] : targets) {
        const auto* cm = model_.byName(node);
        if (cm == nullptr) continue;
        if (isActionTarget(*this, node, param)) continue;
        bool hasLane = false;
        for (const auto& l : cm->automation) hasLane = hasLane || l.propertyName == param;
        if (hasLane) continue;
        const double v = liveParamValue(node, param);
        capturePointAt(node, param, v, v, false, beat);
        any = true;
    }
    if (any) syncAutomation();
}

void EngineHost::capturePoint(const std::string& organism, const std::string& param,
                              double lo, double hi, bool isRange,
                              double prevLo, double prevHi) {
    if (!(capturing_ && playing_) || derivedControl_) return;
    const double beat = positionBeats();
    const auto key = std::make_pair(organism, param);
    const auto it = capturePass_.find(key);
    if (it != capturePass_.end() && beat > it->second.lastBeat) {
        if (auto* c = model_.byName(organism))
            for (auto& l : c->automation)
                if (l.propertyName == param) {
                    const double from = it->second.lastBeat;
                    l.points.erase(std::remove_if(l.points.begin(), l.points.end(),
                                   [&](const AutomationBreakpoint& b) {
                                       return b.beat > from && b.beat <= beat;
                                   }), l.points.end());
                    break;
                }
    }
    if (it == capturePass_.end()) {
        double from = beat;
        if (captureStartBeat_ < beat - kBeatEps && !std::isnan(prevLo)) {
            capturePointAt(organism, param, prevLo, std::isnan(prevHi) ? prevLo : prevHi,
                           isRange, captureStartBeat_);
            from = captureStartBeat_;
        }
        capturePass_[key] = {from, beat, beat, lo, hi, isRange};
    }
    else {
        auto& pass = it->second;
        pass.firstBeat = std::min(pass.firstBeat, beat);
        pass.lastBeat = beat;
        pass.highBeat = std::max(pass.highBeat, beat);
        pass.lo = lo; pass.hi = hi; pass.isRange = isRange;
    }
    capturePointAt(organism, param, lo, hi, isRange, beat);
    syncAutomation();
}

void EngineHost::extendLatchPasses() {
    if (!latch_ || !capturing_ || !playing_ || capturePass_.empty()) return;
    const double now = positionBeats();
    bool any = false;
    for (auto& [key, pass] : capturePass_) {
        if (now <= pass.lastBeat + kBeatEps) continue;
        eraseSpan(model_.byName(key.first), key.second, pass.lastBeat, now);
        capturePointAt(key.first, key.second, pass.lo, pass.hi, pass.isRange, now);
        pass.highBeat = std::max(pass.highBeat, now);
        any = true;
    }
    if (any) syncAutomation();
}

void EngineHost::endCapturePasses() {
    if (capturePass_.empty()) return;
    const double stopBeat = positionBeats();
    if (latch_)
        for (const auto& [key, pass] : capturePass_) {
            if (stopBeat <= pass.lastBeat) continue;
            eraseSpan(model_.byName(key.first), key.second,
                      pass.lastBeat, stopBeat);
            capturePointAt(key.first, key.second, pass.lo, pass.hi, pass.isRange, stopBeat);
        }
    for (const auto& [key, pass] : capturePass_)
        perfbox::addSpan(model_.perfBoxes, key.first,
                         pass.firstBeat, std::max({stopBeat, pass.lastBeat, pass.highBeat}));
    capturePass_.clear();
    dirty_ = true;
    syncAutomation();
}

void EngineHost::noteTouch(const std::string& organism, const std::string& param) {
    if (!playing_ || derivedControl_ || noLatch_) return;
    if (!touched_.emplace(organism, param).second) return;
    if (auto* c = model_.byName(organism))
        for (auto& l : c->automation)
            if (l.propertyName == param) { syncAutomation(); return; }
}

void EngineHost::clearTouches() {
    if (touched_.empty()) return;
    touched_.clear();
    syncAutomation();
}

void EngineHost::applyStateAt(double beat) {
    if (!graph_) return;
    for (auto& c : model_.organisms) {
        auto* node = graph_->find(c.name);
        if (!node) continue;
        for (auto& lane : c.automation) {
            if (lane.points.empty() || lane.kind == "trigger") continue;
            auto* p = node->params.byName(lane.propertyName);
            if (!p) continue;
            const double lo = evalAt(lane.points, beat, false);
            const double hi = lane.kind == "range" ? evalAt(lane.points, beat, true) : lo;
            rtStoreWord(p->value, lo);
            rtStoreWord(p->rangeMin, lo);
            rtStoreWord(p->rangeMax, hi);
        }
    }
    if (metapad().hasMorphPath()) {
        metapad().applyPathAt(beat);
        metapad().replayRecallAt(beat);
    }
    ++liveControlGen_;
}

void EngineHost::capturePointAt(const std::string& organism, const std::string& param,
                                double lo, double hi, bool isRange, double beat) {
    auto* c = model_.byName(organism);
    if (!c) return;
    bool trig = false;
    for (const auto& d : schemaFor(c->classRaw))
        if (d.name == param) { trig = d.isTrigger; break; }
    if (trig && lo < 0.5) return;
    auto* lane = findLane(*c, param);
    if (!lane) {
        AutomationLane nl;
        nl.propertyName = param;
        nl.propertyIndex = paramIndex(*c, param);
        nl.kind = trig ? "trigger" : isRange ? "range"
                : isClockPseudo(c->displayClass) && isHeldClockParam(param) ? "step" : "double";
        c->automation.push_back(std::move(nl));
        lane = &c->automation.back();
        AutomationView view;
        view.organismName = organism;
        view.propertyName = param;
        view.propertyIndex = paramIndex(*c, param);
        view.index = (int) model_.automationViews.size();
        model_.automationViews.push_back(view);
        ++laneStamp_;
    }
    if (trig && lane->kind != "trigger") lane->kind = "trigger";
    const double eps = kBeatEps;
    auto& pts = lane->points;
    pts.erase(std::remove_if(pts.begin(), pts.end(),
              [&](const AutomationBreakpoint& b) { return std::abs(b.beat - beat) < eps; }), pts.end());
    if (trig) pts.push_back({beat, 0.0, 0.0});
    else pts.push_back({beat, std::min(lo, hi), std::max(lo, hi)});
    sortLane(*lane);
    dirty_ = true;
}

}
