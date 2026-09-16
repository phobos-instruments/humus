// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace hum {

namespace {
AutomationLane* findLane(OrganismModel& c, const std::string& param) {
    for (auto& l : c.automation) if (l.propertyName == param) return &l;
    return nullptr;
}
void sortLane(AutomationLane& l) {
    std::sort(l.points.begin(), l.points.end(),
              [](const AutomationBreakpoint& a, const AutomationBreakpoint& b) { return a.beat < b.beat; });
}
}

void AutomationHost::setLoop(double startBeat, double endBeat, bool enabled) {
    doc_.document().clock.loopStart = startBeat;
    doc_.document().clock.loopEnd = endBeat;
    doc_.document().clock.loopEnabled = enabled;
    doc_.flagDirty();
    const juce::ScopedLock sl(audio_.graphLock());
    if (audio_.graph()) audio_.graph()->transport().setLoop(startBeat, endBeat, enabled);
}

bool   AutomationHost::loopEnabled() const   { return doc_.document().clock.loopEnabled; }
double AutomationHost::loopStartBeat() const { return doc_.document().clock.loopStart; }
double AutomationHost::loopEndBeat() const   { return doc_.document().clock.loopEnd; }

int AutomationHost::timeSigNumerator() const {
    return timeSigNumeratorOf(doc_.document().clock.timeSignature);
}

Meter AutomationHost::timeSig() const {
    return meterOf(doc_.document().clock.timeSignature);
}

MeterMap AutomationHost::meterMap() const {
    return MeterMap(meterMapOf(doc_.document()));
}

namespace {
void holdPointAt(AutomationLane& lane, double beat, double value) {
    for (auto& p : lane.points)
        if (std::abs(p.beat - beat) < 1.0e-6) { p.value = value; p.valueMax = value; return; }
    lane.points.push_back({beat, value, value, 0.0});
    sortLane(lane);
}
}

bool AutomationHost::capturingLive() const {
    return capture_.capturing() && host_.isPlaying() && !doc_.derivedControl()
           && !meterAutomated();
}

void AutomationHost::setTimeSignature(int numerator, int denominator) {
    const Meter m{numerator, denominator};
    if (!m.valid()) return;
    if (capturingLive() && m != meterAt(host_.positionBeats())) setMeterAutomated(true);
    if (meterAutomated()) {
        auto* clock = doc_.document().byName(nodes_.clockNodeNameIfAny());
        const double at = meterMap().barStartBefore(host_.positionBeats());
        if (auto* beats = clock ? findLane(*clock, kMeterBeatsParam) : nullptr) holdPointAt(*beats, at, m.beats);
        if (auto* unit = clock ? findLane(*clock, kMeterUnitParam) : nullptr) holdPointAt(*unit, at, m.unit);
        capture_.bumpLaneStamp();
    } else {
        doc_.document().clock.timeSignature = makeTimeSignature(m.beats, m.unit);
    }
    doc_.flagDirty();
    doc_.bumpChangeStamp();
    capture_.syncMeter();
    capture_.publishClock();
}

void AutomationHost::clearTimeRange(double from, double to) {
    if (to <= from) return;
    host_.pushUndo();
    for (auto& cm : doc_.document().organisms)
        for (auto& l : cm.automation) {
            auto& p = l.points;
            p.erase(std::remove_if(p.begin(), p.end(),
                    [&](const AutomationBreakpoint& b) { return b.beat >= from && b.beat < to; }), p.end());
        }
    capture_.syncAutomation();
}

void AutomationHost::deleteTimeRange(double from, double to) {
    if (to <= from) return;
    host_.pushUndo();
    const double span = to - from;
    for (auto& cm : doc_.document().organisms)
        for (auto& l : cm.automation) {
            auto& p = l.points;
            p.erase(std::remove_if(p.begin(), p.end(),
                    [&](const AutomationBreakpoint& b) { return b.beat >= from && b.beat < to; }), p.end());
            for (auto& b : p) if (b.beat >= to) b.beat -= span;
            sortLane(l);
        }
    capture_.syncAutomation();
}

void AutomationHost::insertTime(double at, double amount) {
    if (amount <= 0.0) return;
    host_.pushUndo();
    for (auto& cm : doc_.document().organisms)
        for (auto& l : cm.automation) {
            for (auto& b : l.points) if (b.beat >= at) b.beat += amount;
            sortLane(l);
        }
    capture_.syncAutomation();
}

void AutomationHost::copyTimeRange(double from, double to) {
    if (to <= from) return;
    autoClip_.clear();
    autoClipSpan_ = to - from;
    for (auto& cm : doc_.document().organisms)
        for (auto& l : cm.automation) {
            AutoClipLane cl;
            cl.organism = cm.name;
            cl.param = l.propertyName;
            cl.kind = l.kind;
            for (auto& b : l.points)
                if (b.beat >= from && b.beat < to)
                    cl.points.push_back({b.beat - from, b.value, b.valueMax});
            if (!cl.points.empty()) autoClip_.push_back(std::move(cl));
        }
    hasAutoClip_ = !autoClip_.empty();
}

void AutomationHost::cutTimeRange(double from, double to) {
    copyTimeRange(from, to);
    deleteTimeRange(from, to);
}

bool AutomationHost::pasteTimeRange(double at) {
    if (!hasAutoClip_) return false;
    host_.pushUndo();
    insertTime(at, autoClipSpan_);
    for (auto& cl : autoClip_) {
        auto* c = doc_.document().byName(cl.organism);
        if (!c) continue;
        auto* lane = findLane(*c, cl.param);
        if (!lane) continue;
        for (auto& b : cl.points) lane->points.push_back({at + b.beat, b.value, b.valueMax});
        sortLane(*lane);
    }
    capture_.syncAutomation();
    return true;
}

std::vector<AutomationHost::AutoClipLane> AutomationHost::autoClip_;
double AutomationHost::autoClipSpan_ = 0.0;
bool AutomationHost::hasAutoClip_ = false;

}
