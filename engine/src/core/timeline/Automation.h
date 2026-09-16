// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <vector>

namespace hum {

struct AutoPoint {
    double beat = 0.0;
    double value = 0.0;
    double valueMax = 0.0;
    double curve = 0.0;
};

enum class AutoKind { Double, Range, Trigger, Step };

inline double shapeT(double t, double curve) {
    if (curve == 0.0) return t;
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    return std::pow(t, std::pow(2.0, -2.0 * curve));
}

inline double evaluateEnvelope(const std::vector<AutoPoint>& pts, double beat) {
    if (pts.empty()) return 0.0;
    if (beat <= pts.front().beat) return pts.front().value;
    if (beat >= pts.back().beat) return pts.back().value;
    for (size_t i = 1; i < pts.size(); ++i)
        if (beat <= pts[i].beat) {
            const auto& a = pts[i - 1];
            const auto& b = pts[i];
            const double span = b.beat - a.beat;
            const double t = span > 0.0 ? (beat - a.beat) / span : 0.0;
            return a.value + shapeT(t, a.curve) * (b.value - a.value);
        }
    return pts.back().value;
}

inline double evaluateHeld(const std::vector<AutoPoint>& pts, double beat) {
    if (pts.empty()) return 0.0;
    double v = pts.front().value;
    for (const auto& p : pts) {
        if (p.beat > beat) break;
        v = p.value;
    }
    return v;
}

inline double evaluateEnvelopeMax(const std::vector<AutoPoint>& pts, double beat) {
    if (pts.empty()) return 0.0;
    if (beat <= pts.front().beat) return pts.front().valueMax;
    if (beat >= pts.back().beat) return pts.back().valueMax;
    for (size_t i = 1; i < pts.size(); ++i)
        if (beat <= pts[i].beat) {
            const auto& a = pts[i - 1];
            const auto& b = pts[i];
            const double span = b.beat - a.beat;
            const double t = span > 0.0 ? (beat - a.beat) / span : 0.0;
            return a.valueMax + shapeT(t, a.curve) * (b.valueMax - a.valueMax);
        }
    return pts.back().valueMax;
}

inline bool triggerFiredBetween(const std::vector<AutoPoint>& pts, double from, double to) {
    if (to <= from) return false;
    for (const auto& p : pts)
        if (p.beat > from && p.beat <= to) return true;
    return false;
}

struct AutoLane {
    static constexpr int kTempoNode = -2;
    static constexpr int kGrooveNode = -3;
    static constexpr int kGrooveGridNode = -4;

    int node = -1;
    std::string param;
    int paramSlot = -1;
    AutoKind kind = AutoKind::Double;
    bool mute = false;
    bool suspended = false;
    double rest = 0.0;
    double pulse = 1.0;
    std::vector<AutoPoint> points;
};

}
