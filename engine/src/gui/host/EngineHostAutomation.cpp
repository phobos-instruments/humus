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

bool AutomationHost::meterAutomated() const {
    for (auto& cm : doc_.document().organisms)
        if (isClockPseudo(cm.displayClass))
            for (auto& l : cm.automation)
                if (l.propertyName == kMeterBeatsParam) return true;
    return false;
}

void AutomationHost::setMeterAutomated(bool on) {
    auto& m = doc_.document();
    if (on == meterAutomated()) return;
    host_.pushUndo();
    if (on) {
        auto* clock = const_cast<OrganismModel*>(m.byName(nodes_.clockNodeName()));
        const Meter base = timeSig();
        for (const auto& [param, value] : {std::pair{kMeterBeatsParam, (double) base.beats},
                                           std::pair{kMeterUnitParam, (double) base.unit}}) {
            AutomationLane nl;
            nl.propertyName = param;
            nl.propertyIndex = -1;
            nl.kind = "step";
            nl.points.push_back({0.0, value, value});
            clock->automation.push_back(std::move(nl));
            AutomationView view;
            view.organismName = clock->name;
            view.propertyName = param;
            view.propertyIndex = -1;
            view.index = (int) m.automationViews.size();
            m.automationViews.push_back(view);
        }
    } else {
        auto* clock = m.byName(nodes_.clockNodeNameIfAny());
        if (clock == nullptr) return;
        const Meter now = meterAt(host_.positionBeats());
        auto& lanes = clock->automation;
        lanes.erase(std::remove_if(lanes.begin(), lanes.end(),
                    [](const AutomationLane& l) { return isMeterParam(l.propertyName); }),
                    lanes.end());
        auto& views = m.automationViews;
        const std::string name = clock->name;
        views.erase(std::remove_if(views.begin(), views.end(),
                    [&](const AutomationView& v) {
                        return v.organismName == name && isMeterParam(v.propertyName);
                    }), views.end());
        m.clock.timeSignature = makeTimeSignature(now.beats, now.unit);
    }
    capture_.bumpLaneStamp();
    doc_.markDirty();
    capture_.syncAutomation();
    capture_.publishClock();
}

bool AutomationHost::tempoAutomated() const {
    for (auto& cm : doc_.document().organisms)
        if (cm.displayClass == "ClockPseudoSP")
            for (auto& l : cm.automation)
                if (l.propertyName == "Tempo") return true;
    return false;
}

void AutomationHost::setTempoAutomated(bool on) {
    auto& m = doc_.document();
    OrganismModel* clock = nullptr;
    for (auto& cm : m.organisms)
        if (cm.displayClass == "ClockPseudoSP") { clock = &cm; break; }

    if (on) {
        if (tempoAutomated()) return;
        host_.pushUndo();
        if (!clock) clock = const_cast<OrganismModel*>(m.byName(nodes_.clockNodeName()));
        AutomationLane nl;
        nl.propertyName = "Tempo";
        nl.propertyIndex = -1;
        nl.kind = "double";
        nl.points.push_back({0.0, m.clock.tempo, m.clock.tempo});
        clock->automation.push_back(std::move(nl));
        AutomationView view;
        view.organismName = clock->name;
        view.propertyName = "Tempo";
        view.propertyIndex = -1;
        view.index = (int) m.automationViews.size();
        m.automationViews.push_back(view);
    } else {
        if (!clock) return;
        host_.pushUndo();
        auto& lanes = clock->automation;
        lanes.erase(std::remove_if(lanes.begin(), lanes.end(),
                    [](const AutomationLane& l) { return l.propertyName == "Tempo"; }),
                    lanes.end());
        auto& views = m.automationViews;
        const std::string name = clock->name;
        views.erase(std::remove_if(views.begin(), views.end(),
                    [&](const AutomationView& v) {
                        return v.organismName == name && v.propertyName == "Tempo";
                    }), views.end());
    }
    capture_.bumpLaneStamp();
    doc_.markDirty();
    capture_.syncAutomation();
}

bool AutomationHost::isAutomated(const std::string& organism, const std::string& param) const {
    if (auto* c = doc_.document().byName(organism))
        for (auto& l : c->automation) if (l.propertyName == param) return true;
    return false;
}

void AutomationHost::add(const std::string& organism, const std::string& param) {
    auto* c = doc_.document().byName(organism);
    if (!c || findLane(*c, param)) return;
    host_.pushUndo();
    AutomationLane lane;
    lane.propertyName = param;
    lane.propertyIndex = paramIndex(*c, param);
    const Parameter* pr = paramByName(*c, param);
    const bool isRange = pr && pr->isRange;
    const bool clock = isClockPseudo(c->displayClass);
    bool trig = false;
    for (const auto& d : schemaFor(c->classRaw))
        if (d.name == param) { trig = d.isTrigger; break; }
    lane.kind = trig ? "trigger" : isRange ? "range"
              : clock && isHeldClockParam(param) ? "step" : "double";
    const double now = pr ? pr->value : clock ? host_.liveParamValue(organism, param) : 0.0;
    if (!trig)
        lane.points.push_back({0.0, isRange ? pr->rangeMin : now, isRange ? pr->rangeMax : now});
    c->automation.push_back(std::move(lane));

    AutomationView view;
    view.organismName = organism;
    view.propertyName = param;
    view.propertyIndex = paramIndex(*c, param);
    view.index = (int) doc_.document().automationViews.size();
    doc_.document().automationViews.push_back(view);
    capture_.bumpLaneStamp();
    capture_.syncAutomation();
}

void AutomationHost::remove(const std::string& organism, const std::string& param) {
    auto* c = doc_.document().byName(organism);
    if (!c || !findLane(*c, param)) return;
    host_.pushUndo();
    auto& a = c->automation;
    a.erase(std::remove_if(a.begin(), a.end(),
                           [&](const AutomationLane& l) { return l.propertyName == param; }), a.end());
    auto& v = doc_.document().automationViews;
    v.erase(std::remove_if(v.begin(), v.end(), [&](const AutomationView& av) {
        return av.organismName == organism && av.propertyName == param;
    }), v.end());
    capture_.bumpLaneStamp();
    capture_.syncAutomation();
}

void AutomationHost::clearLane(const std::string& organism, const std::string& param,
                               double from, double to) {
    auto* c = doc_.document().byName(organism);
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane || lane->points.empty()) return;
    const bool all = to < from;
    host_.pushUndo();
    lane->points.erase(std::remove_if(lane->points.begin(), lane->points.end(),
                       [&](const AutomationBreakpoint& b) {
                           return all || (b.beat >= from && b.beat <= to);
                       }), lane->points.end());
    doc_.flagDirty();
    capture_.bumpLaneStamp();
    capture_.syncAutomation();
}

void AutomationHost::clearOrganism(const std::string& organism, bool deleteLanes) {
    auto* c = doc_.document().byName(organism);
    bool hasBoxes = false;
    for (const auto& b : doc_.document().perfBoxes)
        if (b.organism == organism) { hasBoxes = true; break; }
    if ((c == nullptr || c->automation.empty()) && !hasBoxes) return;
    host_.pushUndo();
    if (c != nullptr) {
        if (deleteLanes) {
            c->automation.clear();
            auto& v = doc_.document().automationViews;
            v.erase(std::remove_if(v.begin(), v.end(), [&](const AutomationView& av) {
                return av.organismName == organism;
            }), v.end());
        } else {
            for (auto& l : c->automation) l.points.clear();
        }
    }
    auto& bx = doc_.document().perfBoxes;
    bx.erase(std::remove_if(bx.begin(), bx.end(), [&](const PerformanceBox& b) {
        return b.organism == organism;
    }), bx.end());
    doc_.flagDirty();
    capture_.bumpLaneStamp();
    capture_.syncAutomation();
}

void AutomationHost::addPoint(const std::string& organism, const std::string& param,
                              double beat, double value) {
    auto* c = doc_.document().byName(organism);
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points.push_back({beat, value, value});
    sortLane(*lane);
    capture_.syncAutomation();
}

int AutomationHost::movePoint(const std::string& organism, const std::string& param,
                              int index, double beat, double value) {
    auto* c = doc_.document().byName(organism);
    if (!c) return index;
    auto* lane = findLane(*c, param);
    if (!lane || index < 0 || index >= (int) lane->points.size()) return index;
    auto& pts = lane->points;
    pts.erase(pts.begin() + index);
    int ins = 0;
    while (ins < (int) pts.size() && pts[(size_t) ins].beat < beat) ++ins;
    pts.insert(pts.begin() + ins, {beat, value, value});
    capture_.syncAutomation();
    return ins;
}

void AutomationHost::deletePoint(const std::string& organism, const std::string& param, int index) {
    auto* c = doc_.document().byName(organism);
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane || index < 0 || index >= (int) lane->points.size()) return;
    lane->points.erase(lane->points.begin() + index);
    capture_.syncAutomation();
}

void AutomationHost::setPoints(const std::string& organism, const std::string& param,
                               const std::vector<AutomationBreakpoint>& points) {
    auto* c = doc_.document().byName(organism);
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points = points;
    capture_.syncAutomation();
}

void AutomationHost::setLaneMute(const std::string& organism, const std::string& param, bool mute) {
    auto* c = doc_.document().byName(organism);
    if (!c) return;
    if (auto* lane = findLane(*c, param)) { lane->mute = mute; capture_.syncAutomation(); }
}

std::string AutomationHost::laneKind(const std::string& organism, const std::string& param) const {
    if (auto* c = doc_.document().byName(organism))
        for (auto& l : c->automation) if (l.propertyName == param) return l.kind;
    return "double";
}

void AutomationHost::addRangePoint(const std::string& organism, const std::string& param,
                                   double beat, double lo, double hi) {
    auto* c = doc_.document().byName(organism);
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points.push_back({beat, std::min(lo, hi), std::max(lo, hi)});
    sortLane(*lane);
    capture_.syncAutomation();
}

int AutomationHost::moveRangePoint(const std::string& organism, const std::string& param,
                                   int index, double beat, double lo, double hi) {
    auto* c = doc_.document().byName(organism);
    if (!c) return index;
    auto* lane = findLane(*c, param);
    if (!lane || index < 0 || index >= (int) lane->points.size()) return index;
    auto& pts = lane->points;
    pts.erase(pts.begin() + index);
    int ins = 0;
    while (ins < (int) pts.size() && pts[(size_t) ins].beat < beat) ++ins;
    pts.insert(pts.begin() + ins, {beat, std::min(lo, hi), std::max(lo, hi)});
    capture_.syncAutomation();
    return ins;
}

void AutomationHost::setCurve(const std::string& organism, const std::string& param,
                             int index, double curve) {
    auto* c = doc_.document().byName(organism);
    if (c == nullptr) return;
    auto* lane = findLane(*c, param);
    if (lane == nullptr || index < 0 || index >= (int) lane->points.size()) return;
    const double v = juce::jlimit(-1.0, 1.0, curve);
    if (lane->points[(size_t) index].curve == v) return;
    lane->points[(size_t) index].curve = v;
    doc_.flagDirty();
    capture_.syncAutomation();
}

double AutomationHost::curveAt(const std::string& organism, const std::string& param,
                               int index) const {
    if (const auto* c = doc_.document().byName(organism))
        for (const auto& l : c->automation)
            if (l.propertyName == param && index >= 0 && index < (int) l.points.size())
                return l.points[(size_t) index].curve;
    return 0.0;
}

void AutomationHost::addTriggerPoint(const std::string& organism, const std::string& param, double beat) {
    auto* c = doc_.document().byName(organism);
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points.push_back({beat, 0.0, 0.0});
    sortLane(*lane);
    capture_.syncAutomation();
}

bool AutomationHost::isHeld(const std::string& organism, const std::string& param) const {
    return capture_.touches().count({organism, param}) > 0;
}

bool AutomationHost::anyHeld(const std::string& organism) const {
    for (const auto& t : capture_.touches())
        if (t.first == organism) return true;
    return false;
}

void AutomationHost::release(const std::string& organism, const std::string& param) {
    if (capture_.touches().erase({organism, param}) == 0) return;
    capture_.syncAutomation();
    doc_.bumpLiveControl();
}

void AutomationHost::releaseAll() { capture_.clearTouches(); }

const std::vector<PerformanceBox>& AutomationHost::boxes() const {
    return doc_.document().perfBoxes;
}

void AutomationHost::moveBox(int index, double deltaBeats) {
    if (index < 0 || index >= (int) doc_.document().perfBoxes.size()) return;
    host_.pushUndo();
    perfbox::moveBox(doc_.document(), index, deltaBeats);
    doc_.flagDirty();
    capture_.syncAutomation();
}

int AutomationHost::splitBox(int index, double atBeat) {
    if (index < 0 || index >= (int) doc_.document().perfBoxes.size()) return -1;
    host_.pushUndo();
    const int n = perfbox::splitBox(doc_.document(), index, atBeat);
    if (n >= 0) { doc_.flagDirty(); capture_.syncAutomation(); }
    return n;
}

void AutomationHost::trimBox(int index, double newStart, double newEnd) {
    if (index < 0 || index >= (int) doc_.document().perfBoxes.size()) return;
    host_.pushUndo();
    perfbox::trimBox(doc_.document(), index, newStart, newEnd);
    doc_.flagDirty();
    capture_.syncAutomation();
}

int AutomationHost::duplicateBox(int index, double atBeat) {
    if (index < 0 || index >= (int) doc_.document().perfBoxes.size()) return -1;
    host_.pushUndo();
    const int n = perfbox::duplicateBox(doc_.document(), index, atBeat);
    doc_.flagDirty();
    capture_.syncAutomation();
    return n;
}

void AutomationHost::deleteBox(int index) {
    if (index < 0 || index >= (int) doc_.document().perfBoxes.size()) return;
    host_.pushUndo();
    perfbox::deleteBox(doc_.document(), index);
    doc_.flagDirty();
    capture_.syncAutomation();
}

void AutomationHost::setMasterRecord(bool on) {
    masterRecord_ = on;
}

bool AutomationHost::isMasterRecord() const { return masterRecord_; }

}
