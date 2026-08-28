#include "gui/EngineHost.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include "core/ParamSchema.h"
#include "core/PerfBox.h"
#include "gui/ControlDefaults.h"
#include "core/RtWord.h"

namespace hum {

namespace {
AutomationLane* findLane(OrganismModel& c, const std::string& param) {
    for (auto& l : c.automation) if (l.propertyName == param) return &l;
    return nullptr;
}
const Parameter* paramByName(const OrganismModel& c, const std::string& param) {
    for (auto& p : c.properties) if (p.name == param) return &p;
    return nullptr;
}
int paramIndex(const OrganismModel& c, const std::string& param) {
    for (auto& p : c.properties) if (p.name == param) return p.index;
    return -1;
}
constexpr double kBeatEps = 1.0 / 96.0;

void sortLane(AutomationLane& l) {
    std::sort(l.points.begin(), l.points.end(),
              [](const AutomationBreakpoint& a, const AutomationBreakpoint& b) { return a.beat < b.beat; });
}
}

void EngineHost::syncAutomation() {
    if (!graph_) return;
    std::vector<AutoLane> lanes;
    for (auto& cm : model_.organisms) {
        const bool clockPseudo = cm.displayClass == "ClockPseudoSP";
        const int node = clockPseudo ? AutoLane::kTempoNode : graph_->indexOf(cm.name);
        if (node < 0 && !clockPseudo) continue;
        if (clockPseudo) {
            bool hasTempo = false;
            for (auto& ml : cm.automation) hasTempo = hasTempo || ml.propertyName == "Tempo";
            if (!hasTempo) continue;
        }
        for (auto& ml : cm.automation) {
            if (clockPseudo && ml.propertyName != "Tempo") continue;
            AutoLane lane;
            lane.node = node;
            lane.param = ml.propertyName;
            lane.mute = ml.mute;
            lane.suspended = playing_ && touched_.count({cm.name, ml.propertyName}) > 0;
            lane.kind = ml.kind == "range" ? AutoKind::Range
                      : ml.kind == "trigger" ? AutoKind::Trigger : AutoKind::Double;
            if (lane.kind == AutoKind::Trigger)
                for (const auto& d : schemaFor(cm.classRaw))
                    if (d.name == ml.propertyName) {
                        lane.rest = d.min;
                        lane.pulse = d.max;
                        break;
                    }
            for (auto& bp : ml.points)
                lane.points.push_back({bp.beat, bp.value, bp.valueMax, bp.curve});
            lanes.push_back(std::move(lane));
        }
    }
    const juce::ScopedLock sl(lock_);
    graph_->setAutomation(std::move(lanes));
}

bool AutomationHost::tempoAutomated() const {
    for (auto& cm : host_.model_.organisms)
        if (cm.displayClass == "ClockPseudoSP")
            for (auto& l : cm.automation)
                if (l.propertyName == "Tempo") return true;
    return false;
}

std::string EngineHost::clockNodeName() {
    for (auto& cm : model_.organisms)
        if (cm.displayClass == "ClockPseudoSP") return cm.name;
    OrganismModel cm;
    cm.name = "Clock";
    while (model_.byName(cm.name) != nullptr) cm.name += "_";
    cm.classRaw = "ClockPseudoSP";
    cm.displayClass = "ClockPseudoSP";
    model_.organisms.push_back(std::move(cm));
    return model_.organisms.back().name;
}

void AutomationHost::setTempoAutomated(bool on) {
    auto& m = host_.model_;
    OrganismModel* clock = nullptr;
    for (auto& cm : m.organisms)
        if (cm.displayClass == "ClockPseudoSP") { clock = &cm; break; }

    if (on) {
        if (tempoAutomated()) return;
        host_.pushUndo();
        if (!clock) clock = const_cast<OrganismModel*>(m.byName(host_.clockNodeName()));
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
    ++host_.laneStamp_;
    host_.markDirty();
    host_.syncAutomation();
}

bool AutomationHost::isAutomated(const std::string& organism, const std::string& param) const {
    if (auto* c = host_.model_.byName(organism))
        for (auto& l : c->automation) if (l.propertyName == param) return true;
    return false;
}

void AutomationHost::add(const std::string& organism, const std::string& param) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c || findLane(*c, param)) return;
    host_.pushUndo();
    AutomationLane lane;
    lane.propertyName = param;
    lane.propertyIndex = paramIndex(*c, param);
    const Parameter* pr = paramByName(*c, param);
    const bool isRange = pr && pr->isRange;
    bool trig = false;
    for (const auto& d : schemaFor(c->classRaw))
        if (d.name == param) { trig = d.isTrigger; break; }
    lane.kind = trig ? "trigger" : isRange ? "range" : "double";
    if (!trig)
        lane.points.push_back({0.0, isRange ? (pr ? pr->rangeMin : 0.0)
                                            : (pr ? pr->value : 0.0),
                               isRange ? (pr ? pr->rangeMax : 0.0)
                                       : (pr ? pr->value : 0.0)});
    c->automation.push_back(std::move(lane));

    AutomationView view;
    view.organismName = organism;
    view.propertyName = param;
    view.propertyIndex = paramIndex(*c, param);
    view.index = (int) host_.model_.automationViews.size();
    host_.model_.automationViews.push_back(view);
    ++host_.laneStamp_;
    host_.syncAutomation();
}

void AutomationHost::remove(const std::string& organism, const std::string& param) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c || !findLane(*c, param)) return;
    host_.pushUndo();
    auto& a = c->automation;
    a.erase(std::remove_if(a.begin(), a.end(),
                           [&](const AutomationLane& l) { return l.propertyName == param; }), a.end());
    auto& v = host_.model_.automationViews;
    v.erase(std::remove_if(v.begin(), v.end(), [&](const AutomationView& av) {
        return av.organismName == organism && av.propertyName == param;
    }), v.end());
    ++host_.laneStamp_;
    host_.syncAutomation();
}

void AutomationHost::clearLane(const std::string& organism, const std::string& param,
                               double from, double to) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane || lane->points.empty()) return;
    const bool all = to < from;
    host_.pushUndo();
    lane->points.erase(std::remove_if(lane->points.begin(), lane->points.end(),
                       [&](const AutomationBreakpoint& b) {
                           return all || (b.beat >= from && b.beat <= to);
                       }), lane->points.end());
    host_.dirty_ = true;
    ++host_.laneStamp_;
    host_.syncAutomation();
}

void AutomationHost::clearOrganism(const std::string& organism, bool deleteLanes) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    bool hasBoxes = false;
    for (const auto& b : host_.model_.perfBoxes)
        if (b.organism == organism) { hasBoxes = true; break; }
    if ((c == nullptr || c->automation.empty()) && !hasBoxes) return;
    host_.pushUndo();
    if (c != nullptr) {
        if (deleteLanes) {
            c->automation.clear();
            auto& v = host_.model_.automationViews;
            v.erase(std::remove_if(v.begin(), v.end(), [&](const AutomationView& av) {
                return av.organismName == organism;
            }), v.end());
        } else {
            for (auto& l : c->automation) l.points.clear();
        }
    }
    auto& bx = host_.model_.perfBoxes;
    bx.erase(std::remove_if(bx.begin(), bx.end(), [&](const PerformanceBox& b) {
        return b.organism == organism;
    }), bx.end());
    host_.dirty_ = true;
    ++host_.laneStamp_;
    host_.syncAutomation();
}

void AutomationHost::addPoint(const std::string& organism, const std::string& param,
                              double beat, double value) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points.push_back({beat, value, value});
    sortLane(*lane);
    host_.syncAutomation();
}

int AutomationHost::movePoint(const std::string& organism, const std::string& param,
                              int index, double beat, double value) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return index;
    auto* lane = findLane(*c, param);
    if (!lane || index < 0 || index >= (int) lane->points.size()) return index;
    auto& pts = lane->points;
    pts.erase(pts.begin() + index);
    int ins = 0;
    while (ins < (int) pts.size() && pts[(size_t) ins].beat < beat) ++ins;
    pts.insert(pts.begin() + ins, {beat, value, value});
    host_.syncAutomation();
    return ins;
}

void AutomationHost::deletePoint(const std::string& organism, const std::string& param, int index) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane || index < 0 || index >= (int) lane->points.size()) return;
    lane->points.erase(lane->points.begin() + index);
    host_.syncAutomation();
}

void AutomationHost::setPoints(const std::string& organism, const std::string& param,
                               const std::vector<AutomationBreakpoint>& points) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points = points;
    host_.syncAutomation();
}

void AutomationHost::setLaneMute(const std::string& organism, const std::string& param, bool mute) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return;
    if (auto* lane = findLane(*c, param)) { lane->mute = mute; host_.syncAutomation(); }
}

std::string AutomationHost::laneKind(const std::string& organism, const std::string& param) const {
    if (auto* c = host_.model_.byName(organism))
        for (auto& l : c->automation) if (l.propertyName == param) return l.kind;
    return "double";
}

void AutomationHost::addRangePoint(const std::string& organism, const std::string& param,
                                   double beat, double lo, double hi) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points.push_back({beat, std::min(lo, hi), std::max(lo, hi)});
    sortLane(*lane);
    host_.syncAutomation();
}

int AutomationHost::moveRangePoint(const std::string& organism, const std::string& param,
                                   int index, double beat, double lo, double hi) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return index;
    auto* lane = findLane(*c, param);
    if (!lane || index < 0 || index >= (int) lane->points.size()) return index;
    auto& pts = lane->points;
    pts.erase(pts.begin() + index);
    int ins = 0;
    while (ins < (int) pts.size() && pts[(size_t) ins].beat < beat) ++ins;
    pts.insert(pts.begin() + ins, {beat, std::min(lo, hi), std::max(lo, hi)});
    host_.syncAutomation();
    return ins;
}

void AutomationHost::setCurve(const std::string& organism, const std::string& param,
                             int index, double curve) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (c == nullptr) return;
    auto* lane = findLane(*c, param);
    if (lane == nullptr || index < 0 || index >= (int) lane->points.size()) return;
    const double v = juce::jlimit(-1.0, 1.0, curve);
    if (lane->points[(size_t) index].curve == v) return;
    lane->points[(size_t) index].curve = v;
    host_.dirty_ = true;
    host_.syncAutomation();
}

double AutomationHost::curveAt(const std::string& organism, const std::string& param,
                               int index) const {
    if (const auto* c = host_.model_.byName(organism))
        for (const auto& l : c->automation)
            if (l.propertyName == param && index >= 0 && index < (int) l.points.size())
                return l.points[(size_t) index].curve;
    return 0.0;
}

void AutomationHost::addTriggerPoint(const std::string& organism, const std::string& param, double beat) {
    auto* c = const_cast<OrganismModel*>(host_.model_.byName(organism));
    if (!c) return;
    auto* lane = findLane(*c, param);
    if (!lane) return;
    lane->points.push_back({beat, 0.0, 0.0});
    sortLane(*lane);
    host_.syncAutomation();
}

void EngineHost::pinPerformanceControls() {
    if (!playing_) return;
    const double beat = captureStartBeat_;
    std::set<std::pair<std::string, std::string>> targets;
    for (const auto& e : midiMap_.entries())      targets.insert({e.organism, e.param});
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
        if (auto* c = const_cast<OrganismModel*>(model_.byName(organism)))
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
        capturePass_[key] = {from, beat, lo, hi, isRange};
    }
    else {
        auto& pass = it->second;
        pass.firstBeat = std::min(pass.firstBeat, beat);
        pass.lastBeat = beat;
        pass.lo = lo; pass.hi = hi; pass.isRange = isRange;
    }
    capturePointAt(organism, param, lo, hi, isRange, beat);
    syncAutomation();
}

static void eraseSpan(OrganismModel* c, const std::string& param, double from, double to) {
    if (c == nullptr) return;
    for (auto& l : c->automation)
        if (l.propertyName == param) {
            l.points.erase(std::remove_if(l.points.begin(), l.points.end(),
                           [&](const AutomationBreakpoint& b) {
                               return b.beat > from && b.beat <= to;
                           }), l.points.end());
            break;
        }
}

void EngineHost::extendLatchPasses() {
    if (!latch_ || !capturing_ || !playing_ || capturePass_.empty()) return;
    const double now = positionBeats();
    bool any = false;
    for (const auto& [key, pass] : capturePass_) {
        if (now <= pass.lastBeat + kBeatEps) continue;
        eraseSpan(const_cast<OrganismModel*>(model_.byName(key.first)), key.second, pass.lastBeat, now);
        capturePointAt(key.first, key.second, pass.lo, pass.hi, pass.isRange, now);
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
            eraseSpan(const_cast<OrganismModel*>(model_.byName(key.first)), key.second,
                      pass.lastBeat, stopBeat);
            capturePointAt(key.first, key.second, pass.lo, pass.hi, pass.isRange, stopBeat);
        }
    for (const auto& [key, pass] : capturePass_)
        perfbox::addSpan(model_.perfBoxes, key.first,
                         pass.firstBeat, std::max(stopBeat, pass.lastBeat));
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

bool AutomationHost::isHeld(const std::string& organism, const std::string& param) const {
    return host_.touched_.count({organism, param}) > 0;
}

bool AutomationHost::anyHeld(const std::string& organism) const {
    for (const auto& t : host_.touched_)
        if (t.first == organism) return true;
    return false;
}

void AutomationHost::release(const std::string& organism, const std::string& param) {
    if (host_.touched_.erase({organism, param}) == 0) return;
    host_.syncAutomation();
    ++host_.liveControlGen_;
}

void AutomationHost::releaseAll() { host_.clearTouches(); }

void EngineHost::clearTouches() {
    if (touched_.empty()) return;
    touched_.clear();
    syncAutomation();
}

namespace {
double evalAt(const std::vector<AutomationBreakpoint>& pts, double beat, bool useMax) {
    if (pts.empty()) return 0.0;
    auto v = [&](const AutomationBreakpoint& p) { return useMax ? p.valueMax : p.value; };
    if (beat <= pts.front().beat) return v(pts.front());
    if (beat >= pts.back().beat) return v(pts.back());
    for (size_t i = 1; i < pts.size(); ++i)
        if (pts[i].beat >= beat) {
            const auto& a = pts[i - 1]; const auto& b = pts[i];
            const double t = b.beat > a.beat ? (beat - a.beat) / (b.beat - a.beat) : 0.0;
            return v(a) + (v(b) - v(a)) * shapeT(t, a.curve);
        }
    return v(pts.back());
}
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
    auto* c = const_cast<OrganismModel*>(model_.byName(organism));
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
        nl.kind = trig ? "trigger" : isRange ? "range" : "double";
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

const std::vector<PerformanceBox>& AutomationHost::boxes() const {
    return host_.model_.perfBoxes;
}

void AutomationHost::moveBox(int index, double deltaBeats) {
    if (index < 0 || index >= (int) host_.model_.perfBoxes.size()) return;
    host_.pushUndo();
    perfbox::moveBox(host_.model_, index, deltaBeats);
    host_.dirty_ = true;
    host_.syncAutomation();
}

int AutomationHost::splitBox(int index, double atBeat) {
    if (index < 0 || index >= (int) host_.model_.perfBoxes.size()) return -1;
    host_.pushUndo();
    const int n = perfbox::splitBox(host_.model_, index, atBeat);
    if (n >= 0) { host_.dirty_ = true; host_.syncAutomation(); }
    return n;
}

void AutomationHost::trimBox(int index, double newStart, double newEnd) {
    if (index < 0 || index >= (int) host_.model_.perfBoxes.size()) return;
    host_.pushUndo();
    perfbox::trimBox(host_.model_, index, newStart, newEnd);
    host_.dirty_ = true;
    host_.syncAutomation();
}

int AutomationHost::duplicateBox(int index, double atBeat) {
    if (index < 0 || index >= (int) host_.model_.perfBoxes.size()) return -1;
    host_.pushUndo();
    const int n = perfbox::duplicateBox(host_.model_, index, atBeat);
    host_.dirty_ = true;
    host_.syncAutomation();
    return n;
}

void AutomationHost::deleteBox(int index) {
    if (index < 0 || index >= (int) host_.model_.perfBoxes.size()) return;
    host_.pushUndo();
    perfbox::deleteBox(host_.model_, index);
    host_.dirty_ = true;
    host_.syncAutomation();
}

void AutomationHost::setMasterRecord(bool on) {
    host_.masterRecord_ = on;
}

bool AutomationHost::isMasterRecord() const { return host_.masterRecord_; }

}
