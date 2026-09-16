// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <vector>

#include "hum/Meter.h"
#include "io/PatchDocument.h"

namespace hum {

inline double heldBreakpoint(const std::vector<AutomationBreakpoint>& pts, double beat,
                             double fallback) {
    if (pts.empty()) return fallback;
    double v = pts.front().value;
    for (const auto& p : pts) {
        if (p.beat > beat) break;
        v = p.value;
    }
    return v;
}

inline std::vector<MeterChange> buildMeterMap(Meter base,
                                              const std::vector<AutomationBreakpoint>* beatsPts,
                                              const std::vector<AutomationBreakpoint>* unitPts) {
    std::vector<double> at{0.0};
    for (const auto* lane : {beatsPts, unitPts})
        if (lane != nullptr)
            for (const auto& p : *lane) at.push_back(std::max(0.0, p.beat));
    std::sort(at.begin(), at.end());
    at.erase(std::unique(at.begin(), at.end()), at.end());

    static const std::vector<AutomationBreakpoint> none;
    std::vector<MeterChange> out;
    for (const double b : at) {
        const double beats = heldBreakpoint(beatsPts ? *beatsPts : none, b, base.beats);
        const double unit = heldBreakpoint(unitPts ? *unitPts : none, b, base.unit);
        const Meter m = Meter::clamped(beats, unit);
        if (!out.empty() && out.back().meter == m) continue;
        out.push_back({b, m});
    }
    return out;
}

inline const std::vector<AutomationBreakpoint>* clockLanePoints(const PatchDocumentModel& model,
                                                                 const char* param) {
    for (const auto& cm : model.organisms) {
        if (!isClockPseudo(cm.displayClass)) continue;
        for (const auto& l : cm.automation)
            if (l.propertyName == param && !l.mute) return &l.points;
    }
    return nullptr;
}

inline std::vector<MeterChange> meterMapOf(const PatchDocumentModel& model) {
    return buildMeterMap(meterOf(model.clock.timeSignature),
                         clockLanePoints(model, kMeterBeatsParam),
                         clockLanePoints(model, kMeterUnitParam));
}

class MeterMap {
public:
    MeterMap() : changes_{{0.0, Meter{}}} {}
    explicit MeterMap(std::vector<MeterChange> changes) : changes_(std::move(changes)) {
        if (changes_.empty()) changes_.push_back({0.0, Meter{}});
        changes_.front().beat = 0.0;
    }
    const std::vector<MeterChange>& changes() const { return changes_; }
    Meter at(double beat) const { return meter::at(changes_.data(), count(), beat); }
    int barAt(double beat) const { return meter::barAt(changes_.data(), count(), beat); }
    double barStart(int barIndex) const { return meter::barStart(changes_.data(), count(), barIndex); }
    double barStartBefore(double beat) const { return meter::barStartBefore(changes_.data(), count(), beat); }
    double nextBarStart(double beat) const { return meter::nextBarStart(changes_.data(), count(), beat); }
    double beatInBar(double beat) const { return meter::beatInBar(changes_.data(), count(), beat); }
    double nearestBarStart(double beat) const {
        const double a = barStartBefore(beat), b = nextBarStart(beat);
        return beat - a <= b - beat ? a : b;
    }

private:
    int count() const { return (int) changes_.size(); }
    std::vector<MeterChange> changes_;
};

}
