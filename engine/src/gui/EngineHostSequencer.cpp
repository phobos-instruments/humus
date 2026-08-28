#include "gui/EngineHost.h"

#include <algorithm>
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
    host_.model_.clock.loopStart = startBeat;
    host_.model_.clock.loopEnd = endBeat;
    host_.model_.clock.loopEnabled = enabled;
    host_.dirty_ = true;
    const juce::ScopedLock sl(host_.lock_);
    if (host_.graph_) host_.graph_->transport().setLoop(startBeat, endBeat, enabled);
}

bool   AutomationHost::loopEnabled() const   { return host_.model_.clock.loopEnabled; }
double AutomationHost::loopStartBeat() const { return host_.model_.clock.loopStart; }
double AutomationHost::loopEndBeat() const   { return host_.model_.clock.loopEnd; }

int AutomationHost::timeSigNumerator() const {
    return timeSigNumeratorOf(host_.model_.clock.timeSignature);
}

void AutomationHost::setTimeSignature(int numerator, int denominator) {
    if (numerator <= 0 || denominator <= 0) return;
    host_.model_.clock.timeSignature = makeTimeSignature(numerator, denominator);
    host_.dirty_ = true;
    const juce::ScopedLock sl(host_.lock_);
    if (host_.graph_) host_.graph_->transport().setBeatsPerBar((double) numerator);
    host_.publishClock();
}

void AutomationHost::clearTimeRange(double from, double to) {
    if (to <= from) return;
    host_.pushUndo();
    for (auto& cm : host_.model_.organisms)
        for (auto& l : cm.automation) {
            auto& p = l.points;
            p.erase(std::remove_if(p.begin(), p.end(),
                    [&](const AutomationBreakpoint& b) { return b.beat >= from && b.beat < to; }), p.end());
        }
    host_.syncAutomation();
}

void AutomationHost::deleteTimeRange(double from, double to) {
    if (to <= from) return;
    host_.pushUndo();
    const double span = to - from;
    for (auto& cm : host_.model_.organisms)
        for (auto& l : cm.automation) {
            auto& p = l.points;
            p.erase(std::remove_if(p.begin(), p.end(),
                    [&](const AutomationBreakpoint& b) { return b.beat >= from && b.beat < to; }), p.end());
            for (auto& b : p) if (b.beat >= to) b.beat -= span;
            sortLane(l);
        }
    host_.syncAutomation();
}

void AutomationHost::insertTime(double at, double amount) {
    if (amount <= 0.0) return;
    host_.pushUndo();
    for (auto& cm : host_.model_.organisms)
        for (auto& l : cm.automation) {
            for (auto& b : l.points) if (b.beat >= at) b.beat += amount;
            sortLane(l);
        }
    host_.syncAutomation();
}

void AutomationHost::copyTimeRange(double from, double to) {
    if (to <= from) return;
    autoClip_.clear();
    autoClipSpan_ = to - from;
    for (auto& cm : host_.model_.organisms)
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
        auto* c = const_cast<OrganismModel*>(host_.model_.byName(cl.organism));
        if (!c) continue;
        auto* lane = findLane(*c, cl.param);
        if (!lane) continue;
        for (auto& b : cl.points) lane->points.push_back({at + b.beat, b.value, b.valueMax});
        sortLane(*lane);
    }
    host_.syncAutomation();
    return true;
}

std::vector<AutomationHost::AutoClipLane> AutomationHost::autoClip_;
double AutomationHost::autoClipSpan_ = 0.0;
bool AutomationHost::hasAutoClip_ = false;

}
