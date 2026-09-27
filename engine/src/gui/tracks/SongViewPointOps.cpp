// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

#include <algorithm>
#include <cmath>

#include "core/timeline/AutoPointOps.h"
#include "gui/common/Localisation.h"

namespace hum {

std::vector<std::pair<int, std::set<int>>> SongView::selectedLanes() const {
    std::vector<std::pair<int, std::set<int>>> out;
    auto take = [this, &out](int slot, const std::set<int>& idx) {
        if (slot >= 0 && slot < (int) slots_.size() && !idx.empty()
            && slots_[(size_t) slot].kind == trackslayout::Kind::AutoLane)
            out.push_back({slot, idx});
    };
    take(selPtSlot_, selPts_);
    for (const auto& [slot, idx] : sidePts_) take(slot, idx);
    return out;
}

int SongView::selectedPointCountForTest() const {
    int n = 0;
    for (const auto& [slot, idx] : selectedLanes()) n += (int) idx.size();
    return n;
}

juce::Point<int> SongView::pointPosForTest(int slot, int index) const {
    const auto& sl = slots_[(size_t) slot];
    const auto* l = laneOfSlot(sl);
    if (l == nullptr || index < 0 || index >= (int) l->points.size()) return {};
    const auto [lo, hi] = laneRange(rows_[(size_t) sl.track], sl.param);
    const auto& pt = l->points[(size_t) index];
    return {(int) beatToX(pt.beat), (int) laneYAtValue(sl, pt.value, lo, hi)};
}

void SongView::sweepPointsForTest(juce::Point<int> from, juce::Point<int> to) {
    const int slot = trackslayout::slotAt(slots_, from.y);
    if (slot < 0 || slots_[(size_t) slot].kind != trackslayout::Kind::AutoLane) return;
    clearPointSelection();
    selPtSlot_ = slot;
    marqueeAnchor_ = from;
    updatePointMarquee(to);
    ptMarquee_ = {};
}

void SongView::copySelectedPoints() {
    pointClipboard_.clear();
    pointClipboardSpan_ = 0.0;
    if (ptRange_.active) {
        for (const int slot : rangeSlots_) {
            const auto* l = laneOfSlot(slots_[(size_t) slot]);
            if (l == nullptr) continue;
            for (const auto& b : autoops::rangeCopied(l->points, ptRange_.from, ptRange_.to))
                pointClipboard_.push_back({b.beat, b.value, b.valueMax, b.curve, l->propertyName});
        }
        pointClipboardSpan_ = ptRange_.to - ptRange_.from;
        return;
    }
    double first = 1e18;
    for (const auto& [slot, idx] : selectedLanes())
        if (const auto* l = laneOfSlot(slots_[(size_t) slot]))
            for (const int i : idx)
                if (i >= 0 && i < (int) l->points.size()) first = std::min(first, l->points[(size_t) i].beat);
    for (const auto& [slot, idx] : selectedLanes()) {
        const auto* l = laneOfSlot(slots_[(size_t) slot]);
        if (l == nullptr) continue;
        for (const int i : idx)
            if (i >= 0 && i < (int) l->points.size()) {
                const auto& b = l->points[(size_t) i];
                pointClipboard_.push_back({b.beat - first, b.value, b.valueMax, b.curve, l->propertyName});
            }
    }
}

bool SongView::pastePoints(double atBeat) {
    int slot = selPtSlot_ >= 0 ? selPtSlot_ : hoverPtSlot_;
    if (slot < 0 && hover_.y >= headerH()) slot = trackslayout::slotAt(slots_, hover_.y);
    return pastePointsAt(slot, atBeat);
}

bool SongView::pastePointsAt(int slot, double atBeat) {
    if (slot < 0 || slot >= (int) slots_.size()
        || slots_[(size_t) slot].kind != trackslayout::Kind::AutoLane || pointClipboard_.empty())
        return false;
    const auto& target = slots_[(size_t) slot];
    const auto& node = rows_[(size_t) target.track];
    std::set<std::string> params;
    for (const auto& c : pointClipboard_) params.insert(c.param);
    const bool oneLane = params.size() <= 1;

    host().beginTransaction();
    host().pushUndo();
    bool any = false;
    for (const auto& param : params) {
        const std::string into = oneLane ? target.param : param;
        const auto* cm = host().model().byName(node);
        const bool exists = cm != nullptr && std::any_of(cm->automation.begin(), cm->automation.end(),
            [&into](const AutomationLane& l) { return l.propertyName == into; });
        if (!exists) continue;
        double span = pointClipboardSpan_;
        std::vector<AutomationBreakpoint> add;
        for (const auto& c : pointClipboard_) {
            if (c.param != param) continue;
            AutomationBreakpoint b;
            b.beat = atBeat + c.beat; b.value = c.value; b.valueMax = c.valueMax; b.curve = c.curve;
            span = std::max(span, c.beat);
            add.push_back(b);
        }
        replaceSpan(node, into, atBeat, atBeat + span, add);
        any = true;
    }
    host().endTransaction();
    clearPointSelection();
    repaintAll();
    return any;
}

bool SongView::applyPointOp(PointOp op) {
    const auto lanes = selectedLanes();
    if (op == PointOp::Duplicate && ptRange_.active && !rangeSlots_.empty()) {
        const int slot = selPtSlot_ >= 0 ? selPtSlot_ : *rangeSlots_.begin();
        const double at = ptRange_.to;
        copySelectedPoints();
        return pastePointsAt(slot, at);
    }
    if (lanes.empty()) return false;
    if (op == PointOp::Duplicate) {
        double first = 1e18, last = -1e18;
        for (const auto& [slot, idx] : lanes)
            if (const auto* l = laneOfSlot(slots_[(size_t) slot]))
                for (const int i : idx)
                    if (i >= 0 && i < (int) l->points.size()) {
                        first = std::min(first, l->points[(size_t) i].beat);
                        last = std::max(last, l->points[(size_t) i].beat);
                    }
        if (last < first) return false;
        const double grid = gridBeats();
        const double span = std::max(grid, std::ceil((last - first) / grid - 1e-9) * grid);
        const int slot = lanes.front().first;
        copySelectedPoints();
        return pastePointsAt(slot, first + span);
    }

    host().beginTransaction();
    host().pushUndo();
    for (const auto& [slot, idx] : lanes) {
        const auto& sl = slots_[(size_t) slot];
        const auto& node = rows_[(size_t) sl.track];
        const auto* l = laneOfSlot(sl);
        if (l == nullptr) continue;
        const auto [lo, hi] = laneRange(node, sl.param);
        autoops::Edited<AutomationBreakpoint> out;
        switch (op) {
            case PointOp::Quantise: out = autoops::quantised(l->points, idx, gridBeats()); break;
            case PointOp::Thin:     out = autoops::thinned(l->points, idx, lo, hi); break;
            case PointOp::Smooth:   out = autoops::smoothed(l->points, idx); break;
            case PointOp::Linear:   out = autoops::curvedBetween(l->points, idx, autoops::kCurveLinear); break;
            case PointOp::Log:      out = autoops::curvedBetween(l->points, idx, autoops::kCurveLog); break;
            case PointOp::Exp:      out = autoops::curvedBetween(l->points, idx, autoops::kCurveExp); break;
            case PointOp::Duplicate: break;
        }
        host().automation().setPoints(node, sl.param, out.points);
        if (slot == selPtSlot_) selPts_ = out.selected;
        else sidePts_[slot] = out.selected;
    }
    host().endTransaction();
    repaintAll();
    return true;
}

void SongView::showPointMenu(juce::Point<int> screen, int slot, double atBeat) {
    const bool some = !selectedLanes().empty() || (ptRange_.active && !rangeSlots_.empty());
    juce::PopupMenu curves;
    curves.addItem(21, tr("tracks-points.linear", "Linear"), some);
    curves.addItem(22, tr("tracks-points.log", "Logarithmic (fast, then slow)"), some);
    curves.addItem(23, tr("tracks-points.exp", "Exponential (slow, then fast)"), some);

    juce::PopupMenu m;
    m.addItem(1, tr("tracks-points.cut", "Cut"), some);
    m.addItem(2, tr("tracks-points.copy", "Copy"), some);
    m.addItem(3, tr("tracks-points.paste-here", "Paste Here"), !pointClipboard_.empty());
    m.addItem(4, tr("tracks-points.duplicate", "Duplicate"), some);
    m.addItem(5, tr("tracks-points.delete", "Delete"), some);
    m.addSeparator();
    m.addItem(11, tr("tracks-points.quantise", "Quantise to Grid"), some);
    m.addItem(12, tr("tracks-points.thin", "Thin Points"), some);
    m.addItem(13, tr("tracks-points.smooth", "Smooth"), some);
    m.addSubMenu(tr("tracks-points.curve", "Curve"), curves, some);
    m.addSeparator();
    m.addItem(31, tr("tracks-pane-input.clear-lane-points", "Clear Lane Points"));
    m.addItem(32, tr("tracks-pane-input.delete-lane", "Delete Lane"));

    const auto node = rows_[(size_t) slots_[(size_t) slot].track];
    const auto param = slots_[(size_t) slot].param;
    juce::Component::SafePointer<SongView> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screen.x, screen.y, 1, 1}),
                    [safe, slot, atBeat, node, param](int r) {
        if (safe == nullptr || r <= 0) return;
        auto& v = *safe;
        switch (r) {
            case 1: v.copySelectedPoints(); v.deleteSelectedPoints(); break;
            case 2: v.copySelectedPoints(); break;
            case 3: v.pastePointsAt(slot, atBeat); break;
            case 4: v.applyPointOp(PointOp::Duplicate); break;
            case 5: v.deleteSelectedPoints(); break;
            case 11: v.applyPointOp(PointOp::Quantise); break;
            case 12: v.applyPointOp(PointOp::Thin); break;
            case 13: v.applyPointOp(PointOp::Smooth); break;
            case 21: v.applyPointOp(PointOp::Linear); break;
            case 22: v.applyPointOp(PointOp::Log); break;
            case 23: v.applyPointOp(PointOp::Exp); break;
            case 31: v.host().automation().clearLane(node, param); v.rebuild(); break;
            case 32: v.host().automation().remove(node, param); v.rebuild(); break;
            default: break;
        }
    });
}

}
