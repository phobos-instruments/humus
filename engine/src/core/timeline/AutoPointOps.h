// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>

namespace hum::autoops {

inline constexpr double kCurveLinear = 0.0;
inline constexpr double kCurveLog = 1.0;
inline constexpr double kCurveExp = -1.0;

template <class Point>
struct Edited {
    std::vector<Point> points;
    std::set<int> selected;
};

template <class Point>
Edited<Point> resorted(std::vector<std::pair<Point, bool>> tagged) {
    std::stable_sort(tagged.begin(), tagged.end(),
                     [](const auto& a, const auto& b) { return a.first.beat < b.first.beat; });
    Edited<Point> out;
    for (auto& [p, sel] : tagged) {
        const bool clash = !out.points.empty() && out.points.back().beat == p.beat;
        if (clash && !sel) continue;
        if (clash) {
            out.selected.erase((int) out.points.size() - 1);
            out.points.pop_back();
        }
        if (sel) out.selected.insert((int) out.points.size());
        out.points.push_back(p);
    }
    return out;
}

template <class Point>
Edited<Point> quantised(const std::vector<Point>& pts, const std::set<int>& sel, double grid,
                        double origin = 0.0) {
    std::vector<std::pair<Point, bool>> tagged;
    for (int i = 0; i < (int) pts.size(); ++i) {
        Point p = pts[(size_t) i];
        const bool on = sel.count(i) != 0;
        if (on && grid > 0.0)
            p.beat = std::max(0.0, origin + std::round((p.beat - origin) / grid) * grid);
        tagged.push_back({p, on});
    }
    return resorted(std::move(tagged));
}

template <class Point>
Edited<Point> smoothed(const std::vector<Point>& pts, const std::set<int>& sel) {
    Edited<Point> out{pts, sel};
    for (const int i : sel) {
        if (i <= 0 || i + 1 >= (int) pts.size()) continue;
        if (sel.count(i - 1) == 0 && sel.count(i + 1) == 0) continue;
        const auto& a = pts[(size_t) i - 1];
        const auto& b = pts[(size_t) i];
        const auto& c = pts[(size_t) i + 1];
        out.points[(size_t) i].value = 0.25 * a.value + 0.5 * b.value + 0.25 * c.value;
        out.points[(size_t) i].valueMax = 0.25 * a.valueMax + 0.5 * b.valueMax + 0.25 * c.valueMax;
    }
    return out;
}

template <class Point>
void keepTurns(const std::vector<Point>& pts, int from, int to, double tolerance, double lo,
               double hi, std::vector<bool>& keep) {
    if (to - from < 2) return;
    const auto& a = pts[(size_t) from];
    const auto& b = pts[(size_t) to];
    const double span = b.beat - a.beat, range = hi > lo ? hi - lo : 1.0;
    int worst = -1;
    double worstBy = tolerance;
    for (int i = from + 1; i < to; ++i) {
        const auto& p = pts[(size_t) i];
        const double t = span > 0.0 ? (p.beat - a.beat) / span : 0.0;
        const double by = std::abs(p.value - (a.value + t * (b.value - a.value))) / range;
        if (by > worstBy) { worstBy = by; worst = i; }
    }
    if (worst < 0) return;
    keep[(size_t) worst] = true;
    keepTurns(pts, from, worst, tolerance, lo, hi, keep);
    keepTurns(pts, worst, to, tolerance, lo, hi, keep);
}

template <class Point>
Edited<Point> thinned(const std::vector<Point>& pts, const std::set<int>& sel, double lo, double hi,
                      double tolerance = 0.01) {
    std::vector<bool> keep(pts.size(), false);
    for (int i = 0; i < (int) pts.size(); ++i) keep[(size_t) i] = sel.count(i) == 0;
    for (auto it = sel.begin(); it != sel.end();) {
        const int from = *it;
        int to = from;
        for (++it; it != sel.end() && *it == to + 1; ++it) to = *it;
        keep[(size_t) from] = keep[(size_t) to] = true;
        keepTurns(pts, from, to, tolerance, lo, hi, keep);
    }
    Edited<Point> out;
    for (int i = 0; i < (int) pts.size(); ++i) {
        if (!keep[(size_t) i]) continue;
        if (sel.count(i) != 0) out.selected.insert((int) out.points.size());
        out.points.push_back(pts[(size_t) i]);
    }
    return out;
}

template <class Point>
Edited<Point> shaped(const std::vector<Point>& pts, const std::set<int>& sel, double curve) {
    Edited<Point> out{pts, sel};
    for (const int i : sel)
        if (i >= 0 && i + 1 < (int) pts.size() && sel.count(i + 1) != 0)
            out.points[(size_t) i].curve = curve;
    if (sel.size() == 1 && *sel.begin() + 1 < (int) pts.size())
        out.points[(size_t) *sel.begin()].curve = curve;
    return out;
}

inline double bent(double t, double curve) {
    if (curve == 0.0) return t;
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    return std::pow(t, std::pow(2.0, -2.0 * curve));
}

template <class Point>
double valueAt(const std::vector<Point>& pts, double beat, bool upper = false) {
    auto of = [upper](const Point& p) { return upper ? p.valueMax : p.value; };
    if (pts.empty()) return 0.0;
    if (beat <= pts.front().beat) return of(pts.front());
    if (beat >= pts.back().beat) return of(pts.back());
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        const auto& a = pts[i];
        const auto& b = pts[i + 1];
        if (beat < a.beat || beat > b.beat) continue;
        const double span = b.beat - a.beat;
        const double t = span > 0.0 ? (beat - a.beat) / span : 0.0;
        return of(a) + bent(t, a.curve) * (of(b) - of(a));
    }
    return of(pts.back());
}

template <class Point>
Point edgeAt(const std::vector<Point>& pts, double beat) {
    Point p{};
    p.beat = beat;
    p.value = valueAt(pts, beat);
    p.valueMax = valueAt(pts, beat, true);
    return p;
}

template <class Point>
std::vector<Point> rangeCopied(const std::vector<Point>& pts, double from, double to) {
    std::vector<Point> out;
    if (pts.empty() || to <= from) return out;
    out.push_back(edgeAt(pts, from));
    for (const auto& p : pts)
        if (p.beat > from && p.beat < to) out.push_back(p);
    out.push_back(edgeAt(pts, to));
    for (size_t i = 0; i + 1 < pts.size(); ++i)
        if (from >= pts[i].beat && from < pts[i + 1].beat) out.front().curve = pts[i].curve;
    for (auto& p : out) p.beat -= from;
    return out;
}

template <class Point>
std::vector<Point> rangeCleared(const std::vector<Point>& pts, double from, double to) {
    if (pts.empty() || to <= from) return pts;
    const Point left = edgeAt(pts, from), right = edgeAt(pts, to);
    std::vector<Point> out;
    for (const auto& p : pts)
        if (p.beat < from) out.push_back(p);
    out.push_back(left);
    out.push_back(right);
    for (const auto& p : pts)
        if (p.beat > to) out.push_back(p);
    return out;
}

template <class Point>
Edited<Point> curvedBetween(const std::vector<Point>& pts, const std::set<int>& sel, double curve) {
    if (sel.size() < 2) return shaped(pts, sel, curve);
    const int first = *sel.begin(), last = *sel.rbegin();
    Edited<Point> out;
    for (int i = 0; i < (int) pts.size(); ++i) {
        if (i > first && i < last) continue;
        if (i == first || i == last) out.selected.insert((int) out.points.size());
        out.points.push_back(pts[(size_t) i]);
        if (i == first) out.points.back().curve = curve;
    }
    return out;
}

template <class Point>
std::set<int> withinBeats(const std::vector<Point>& pts, double from, double to) {
    std::set<int> out;
    for (int i = 0; i < (int) pts.size(); ++i)
        if (pts[(size_t) i].beat >= from && pts[(size_t) i].beat <= to) out.insert(i);
    return out;
}

}
