// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "core/timeline/Automation.h"
#include "io/PatchDocument.h"

namespace hum::lanes {

inline AutomationLane* findLane(OrganismModel& c, const std::string& param) {
    for (auto& l : c.automation) if (l.propertyName == param) return &l;
    return nullptr;
}
inline const Parameter* paramByName(const OrganismModel& c, const std::string& param) {
    for (auto& p : c.properties) if (p.name == param) return &p;
    return nullptr;
}
inline int paramIndex(const OrganismModel& c, const std::string& param) {
    for (auto& p : c.properties) if (p.name == param) return p.index;
    return -1;
}
inline constexpr double kBeatEps = 1.0 / 96.0;

inline void sortLane(AutomationLane& l) {
    std::sort(l.points.begin(), l.points.end(),
              [](const AutomationBreakpoint& a, const AutomationBreakpoint& b) { return a.beat < b.beat; });
}

inline double evalAt(const std::vector<AutomationBreakpoint>& pts, double beat, bool useMax) {
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

inline int clockLaneNode(const std::string& param) {
    if (param == kTempoParam) return AutoLane::kTempoNode;
    if (param == kGrooveParam) return AutoLane::kGrooveNode;
    if (param == kGrooveGridParam) return AutoLane::kGrooveGridNode;
    return -1;
}

inline void eraseSpan(OrganismModel* c, const std::string& param, double from, double to) {
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

}
