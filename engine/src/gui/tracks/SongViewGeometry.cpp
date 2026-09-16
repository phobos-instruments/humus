// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"
#include "core/timeline/Automation.h"
#include "core/graph/PodModel.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include "gui/style/EnvelopePainter.h"

namespace hum {

std::vector<trackslayout::AutoLaneInfo> SongView::lanesOf(int row) const {
    std::vector<trackslayout::AutoLaneInfo> out;
    if (row < 0 || row >= (int) rows_.size()) return out;
    if (const auto* cm = host().model().byName(rows_[(size_t) row]))
        for (const auto& l : cm->automation)
            out.push_back({l.propertyName, l.kind});
    return out;
}

int SongView::rowTop(int row) const {
    const int y = trackslayout::trackY(slots_, row);
    return y >= 0 ? y : headerH() + row * rowH_;
}

std::pair<double, double> SongView::laneRange(const std::string& node,
                                                const std::string& param) const {
    return envpaint::laneRange(host().model().byName(node), param,
                               (int) host().model().metapad.snapshots.size());
}

float SongView::laneYAtValue(const trackslayout::Slot& s, double v, double lo, double hi) const {
    const double span = hi > lo ? hi - lo : 1.0;
    return (float) (s.y + s.h - 3 - (v - lo) / span * (s.h - 6));
}

double SongView::laneValueAtY(const trackslayout::Slot& s, int py, double lo, double hi) const {
    const double span = hi > lo ? hi - lo : 1.0;
    const double v = lo + ((double) (s.y + s.h - 3) - py) / (s.h - 6) * span;
    return juce::jlimit(std::min(lo, hi), std::max(lo, hi), v);
}

int SongView::autoPointAt(const trackslayout::Slot& s, const std::string& node,
                            const std::string& param, juce::Point<int> p) const {
    const auto* cm = host().model().byName(node);
    if (!cm) return -1;
    const hum::AutomationLane* lane = nullptr;
    for (const auto& l : cm->automation) if (l.propertyName == param) { lane = &l; break; }
    if (!lane) return -1;
    const auto [lo, hi] = laneRange(node, param);
    int best = -1; float bestD = 7.0f;
    for (int i = 0; i < (int) lane->points.size(); ++i) {
        const auto& pt = lane->points[(size_t) i];
        const float dx = beatToX(pt.beat) - p.x;
        if (lane->kind == "trigger") {
            const float d = std::abs(dx);
            if (d < bestD) { bestD = d; best = i; }
            continue;
        }
        float dy = laneYAtValue(s, pt.value, lo, hi) - p.y;
        if (lane->kind == "range") {
            const float dyHi = laneYAtValue(s, pt.valueMax, lo, hi) - p.y;
            if (std::abs(dyHi) < std::abs(dy)) dy = dyHi;
        }
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

SongView::AutoGrip SongView::nearerRangeEdge(const trackslayout::Slot& s,
                                                 const std::string& node,
                                                 const std::string& param, int index,
                                                 juce::Point<int> p) const {
    const auto* cm = host().model().byName(node);
    if (cm == nullptr || index < 0) return AutoGrip::RangeLo;
    for (const auto& l : cm->automation) {
        if (l.propertyName != param) continue;
        if (index >= (int) l.points.size()) break;
        const auto& pt = l.points[(size_t) index];
        const auto [lo, hi] = laneRange(node, param);
        const float dLo = std::abs(laneYAtValue(s, pt.value, lo, hi) - p.y);
        const float dHi = std::abs(laneYAtValue(s, pt.valueMax, lo, hi) - p.y);
        return dHi < dLo ? AutoGrip::RangeHi : AutoGrip::RangeLo;
    }
    return AutoGrip::RangeLo;
}

int SongView::rowAt(int y) const {
    const int s = trackslayout::slotAt(slots_, y);
    if (s < 0 || slots_[(size_t) s].kind != trackslayout::Kind::Track) return -1;
    return slots_[(size_t) s].track;
}

int SongView::segmentAt(const trackslayout::Slot& s, const std::string& node,
                          const std::string& param, juce::Point<int> p) const {
    const auto* cm = host().model().byName(node);
    if (cm == nullptr) return -1;
    const AutomationLane* lane = nullptr;
    for (const auto& l : cm->automation) if (l.propertyName == param) { lane = &l; break; }
    if (lane == nullptr || lane->kind != "double" || lane->points.size() < 2) return -1;
    const auto [lo, hi] = laneRange(node, param);
    for (int i = 1; i < (int) lane->points.size(); ++i) {
        const auto& a = lane->points[(size_t) (i - 1)];
        const auto& b = lane->points[(size_t) i];
        const float ax = beatToX(a.beat), bx = beatToX(b.beat);
        if ((float) p.x < ax || (float) p.x > bx || bx <= ax) continue;
        const double t = (p.x - ax) / (double) (bx - ax);
        const double v = a.value + (b.value - a.value) * shapeT(t, a.curve);
        if (std::abs(laneYAtValue(s, v, lo, hi) - p.y) <= 5.0f) return i - 1;
    }
    return -1;
}

juce::Rectangle<int> SongView::clipBounds(int row, const ClipEditor::ClipInfo& ci) const {
    const int x0 = (int) tickToX(ci.startTick);
    const int x1 = (int) tickToX(ci.startTick + ci.lengthTicks);
    return {x0, rowTop(row) + 2, std::max(6, x1 - x0), rowH_ - 5};
}

int SongView::clipAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const {
    leftEdge = rightEdge = false;
    if (row < 0) return -1;
    const auto clips = host().clips().list(rows_[(size_t) row]);
    for (int i = (int) clips.size() - 1; i >= 0; --i) {
        const auto b = clipBounds(row, clips[(size_t) i]);
        if (!b.contains(p)) continue;
        leftEdge = p.x <= b.getX() + 5;
        rightEdge = p.x >= b.getRight() - 5;
        return i;
    }
    return -1;
}

juce::Rectangle<int> SongView::boxBounds(int row, const PerformanceBox& b) const {
    const int y = rowTop(row);
    double s = b.startBeat, e = b.endBeat;
    if (dragBox_ >= 0 && dragBox_ < (int) host().automation().boxes().size()
        && &host().automation().boxes()[(size_t) dragBox_] == &b) {
        s += boxDragDelta_ + boxTrimL_;
        e += boxDragDelta_ + boxTrimR_;
    }
    const int x0 = (int) beatToX(s), x1 = (int) beatToX(e);
    if (const int bs = boxRowSlot(row); bs >= 0) {
        const auto& sl = slots_[(size_t) bs];
        return {x0, sl.y + 2, std::max(10, x1 - x0), sl.h - 4};
    }
    return {x0, y + 2, std::max(10, x1 - x0), rowH_ - 4};
}

int SongView::boxAt(int row, juce::Point<int> p) const {
    bool l = false, r = false;
    return boxAt(row, p, l, r);
}

int SongView::boxAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const {
    leftEdge = rightEdge = false;
    const auto& boxes = host().automation().boxes();
    for (int i = (int) boxes.size() - 1; i >= 0; --i) {
        if (boxes[(size_t) i].organism != rows_[(size_t) row]) continue;
        const auto b = boxBounds(row, boxes[(size_t) i]);
        if (!b.contains(p)) continue;
        leftEdge = p.x <= b.getX() + 5;
        rightEdge = p.x >= b.getRight() - 5;
        return i;
    }
    return -1;
}

}
