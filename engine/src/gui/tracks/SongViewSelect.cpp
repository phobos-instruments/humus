// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <map>

#include "core/graph/PerfBox.h"
#include "gui/style/Colours.h"
#include "gui/tracks/SongView.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "gui/host/TracksHost.h"
#include "core/timeline/AutoPointOps.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

const AutomationLane* SongView::laneOfSlot(const trackslayout::Slot& s) const {
    if (s.track < 0 || s.track >= (int) rows_.size()) return nullptr;
    const auto* cm = host().model().byName(rows_[(size_t) s.track]);
    if (cm == nullptr) return nullptr;
    for (const auto& l : cm->automation)
        if (l.propertyName == s.param) return &l;
    return nullptr;
}

bool SongView::tryBeginPointSelection(const juce::MouseEvent& e, int slot, int hitPoint) {
    if (slot < 0 || slot >= (int) slots_.size()) return false;
    const auto& sl = slots_[(size_t) slot];

    if (hitPoint < 0 && e.x >= kStripW
        && (e.mods.isShiftDown() || effectiveTool() == Tool::Pointer)) {
        if (slot != selPtSlot_) selPts_.clear();
        selPtSlot_ = slot;
        marqueeAnchor_ = e.getPosition();
        ptMarquee_ = juce::Rectangle<int>(marqueeAnchor_, marqueeAnchor_);
        drag_ = Drag::PointMarquee;
        repaintAll();
        return true;
    }

    if (hitPoint >= 0 && slot == selPtSlot_ && selPts_.count(hitPoint) > 0) {
        const auto* l = laneOfSlot(sl);
        if (l == nullptr) return false;
        host().pushUndo();
        groupBase_ = l->points;
        groupSel0_ = selPts_;
        ptRange0_ = ptRange_;
        sideBase_.clear();
        sidePts0_ = sidePts_;
        for (const auto& [side, idx] : sidePts_)
            if (side >= 0 && side < (int) slots_.size())
                if (const auto* sideLane = laneOfSlot(slots_[(size_t) side]))
                    sideBase_[side] = sideLane->points;
        groupBeat0_ = xToBeat((float) e.x);
        const auto [lo, hi] = laneRange(rows_[(size_t) sl.track], sl.param);
        groupVal0_ = laneValueAtY(sl, e.y, lo, hi);
        drag_ = Drag::PointGroup;
        return true;
    }
    return false;
}

void SongView::updatePointMarquee(juce::Point<int> p) {
    ptMarquee_ = juce::Rectangle<int>(marqueeAnchor_, p);
    selPts_.clear();
    sidePts_.clear();
    rangeSlots_.clear();
    ptRange_ = {};
    if (selPtSlot_ < 0 || selPtSlot_ >= (int) slots_.size()) return;

    const bool free = juce::ModifierKeys::currentModifiers.isAltDown();
    const double a = snapBeats(std::max(0.0, xToBeat((float) marqueeAnchor_.x)), free);
    const double b = snapBeats(std::max(0.0, xToBeat((float) p.x)), free);
    ptRange_ = {std::min(a, b), std::max(a, b), std::abs(b - a) > 1.0e-9};
    if (!ptRange_.active) { repaintAll(); return; }

    for (int i = 0; i < (int) slots_.size(); ++i) {
        const auto& sl = slots_[(size_t) i];
        if (sl.kind != trackslayout::Kind::AutoLane || sl.track != slots_[(size_t) selPtSlot_].track
            || ptMarquee_.getBottom() < sl.y || ptMarquee_.getY() > sl.y + sl.h)
            continue;
        const auto* l = laneOfSlot(sl);
        if (l == nullptr) continue;
        rangeSlots_.insert(i);
        auto inside = autoops::withinBeats(l->points, ptRange_.from, ptRange_.to);
        if (i == selPtSlot_) selPts_ = std::move(inside);
        else if (!inside.empty()) sidePts_[i] = std::move(inside);
    }
    repaintAll();
}

void SongView::dragSelectedPoints(const juce::MouseEvent& e) {
    if (selPtSlot_ < 0 || selPtSlot_ >= (int) slots_.size()) return;
    if (groupBase_.empty() || groupSel0_.empty()) return;
    const auto& sl = slots_[(size_t) selPtSlot_];
    const auto& node = rows_[(size_t) sl.track];
    const auto* l = laneOfSlot(sl);
    const bool trigger = l != nullptr && l->kind == "trigger";
    const auto [lo, hi] = laneRange(node, sl.param);

    double db = xToBeat((float) e.x) - groupBeat0_;
    if (!e.mods.isAltDown()) {
        const double grid = gridBeats();
        db = std::round(db / grid) * grid;
    }
    const double dv = trigger ? 0.0 : laneValueAtY(sl, e.y, lo, hi) - groupVal0_;

    std::vector<std::pair<AutomationBreakpoint, bool>> tagged;
    tagged.reserve(groupBase_.size());
    for (int i = 0; i < (int) groupBase_.size(); ++i) {
        AutomationBreakpoint bp = groupBase_[(size_t) i];
        const bool sel = groupSel0_.count(i) > 0;
        if (sel) {
            bp.beat = juce::jmax(0.0, bp.beat + db);
            bp.value = juce::jlimit(lo, hi, bp.value + dv);
            bp.valueMax = juce::jlimit(lo, hi, bp.valueMax + dv);
        }
        tagged.push_back({bp, sel});
    }
    std::stable_sort(tagged.begin(), tagged.end(),
                     [](const auto& a, const auto& b) { return a.first.beat < b.first.beat; });

    std::vector<AutomationBreakpoint> pts;
    pts.reserve(tagged.size());
    selPts_.clear();
    for (int i = 0; i < (int) tagged.size(); ++i) {
        if (tagged[(size_t) i].second) selPts_.insert(i);
        pts.push_back(tagged[(size_t) i].first);
    }
    host().automation().setPoints(node, sl.param, pts);
    if (ptRange0_.active) ptRange_ = {ptRange0_.from + db, ptRange0_.to + db, true};
    for (const auto& [side, base] : sideBase_) {
        if (side < 0 || side >= (int) slots_.size()) continue;
        std::vector<std::pair<AutomationBreakpoint, bool>> moved;
        const auto was = sidePts0_.count(side) != 0 ? sidePts0_.at(side) : std::set<int>{};
        for (int i = 0; i < (int) base.size(); ++i) {
            auto bp = base[(size_t) i];
            const bool on = was.count(i) != 0;
            if (on) bp.beat = juce::jmax(0.0, bp.beat + db);
            moved.push_back({bp, on});
        }
        auto out = autoops::resorted(std::move(moved));
        sidePts_[side] = out.selected;
        host().automation().setPoints(rows_[(size_t) slots_[(size_t) side].track],
                                      slots_[(size_t) side].param, out.points);
    }
    repaintAll();
}

bool SongView::deleteSelectedPoints() {
    const auto lanes = selectedLanes();
    if (lanes.empty() && !(ptRange_.active && !rangeSlots_.empty())) return false;
    host().beginTransaction();
    host().pushUndo();
    if (ptRange_.active) {
        for (const int slot : rangeSlots_) {
            const auto& sl = slots_[(size_t) slot];
            if (const auto* l = laneOfSlot(sl))
                host().automation().setPoints(rows_[(size_t) sl.track], sl.param,
                                              autoops::rangeCleared(l->points, ptRange_.from, ptRange_.to));
        }
    } else {
        for (const auto& [slot, idx] : lanes) {
            const auto& sl = slots_[(size_t) slot];
            const auto* l = laneOfSlot(sl);
            if (l == nullptr) continue;
            std::vector<AutomationBreakpoint> kept;
            for (int i = 0; i < (int) l->points.size(); ++i)
                if (idx.count(i) == 0) kept.push_back(l->points[(size_t) i]);
            host().automation().setPoints(rows_[(size_t) sl.track], sl.param, kept);
        }
    }
    host().endTransaction();
    clearPointSelection();
    return true;
}

void SongView::clearPointSelection() {
    selPts_.clear();
    sidePts_.clear();
    rangeSlots_.clear();
    ptRange_ = {};
    selPtSlot_ = -1;
    ptMarquee_ = {};
    repaintAll();
}

void SongView::paintPointSelection(juce::Graphics& g) {
    if (ptRange_.active) {
        const float x0 = std::max((float) kStripW, beatToX(ptRange_.from)), x1 = beatToX(ptRange_.to);
        for (const int slot : rangeSlots_) {
            if (slot < 0 || slot >= (int) slots_.size() || x1 <= x0) continue;
            const auto& sl = slots_[(size_t) slot];
            g.setColour(Palette::accent.withAlpha(alpha::mist));
            g.fillRect(x0, (float) sl.y, x1 - x0, (float) sl.h - 1.0f);
            g.setColour(Palette::accent.withAlpha(alpha::dim));
            g.fillRect(x0, (float) sl.y, 1.0f, (float) sl.h - 1.0f);
            g.fillRect(x1 - 1.0f, (float) sl.y, 1.0f, (float) sl.h - 1.0f);
        }
    }
    for (const auto& [slot, idx] : selectedLanes()) paintSelectedPointsOf(g, slot, idx);
}

void SongView::paintSelectedPointsOf(juce::Graphics& g, int slot, const std::set<int>& idx) {
    const auto& sl = slots_[(size_t) slot];
    const auto* l = laneOfSlot(sl);
    if (l == nullptr) return;
    const auto [lo, hi] = laneRange(rows_[(size_t) sl.track], sl.param);
    g.setColour(Palette::text);
    for (const int i : idx) {
        if (i < 0 || i >= (int) l->points.size()) continue;
        const auto& bp = l->points[(size_t) i];
        const float x = beatToX(bp.beat);
        if (x < (float) kStripW) continue;
        if (l->kind == "trigger") {
            g.drawRect(x - 2.0f, (float) sl.y + 4.0f, 4.0f, (float) sl.h - 8.0f, 1.0f);
            continue;
        }
        g.drawEllipse(x - 4.5f, laneYAtValue(sl, bp.value, lo, hi) - 4.5f, 9.0f, 9.0f, 1.4f);
        if (l->kind == "range")
            g.drawEllipse(x - 4.5f, laneYAtValue(sl, bp.valueMax, lo, hi) - 4.5f, 9.0f, 9.0f, 1.4f);
    }
}

void SongView::updateClipMarquee(juce::Point<int> p) {
    clipMarquee_ = juce::Rectangle<int>(marqueeAnchor_, p);
    marqueeSelect(clipMarquee_);
    syncTimeSelection();
    repaintAll();
}

std::vector<std::pair<int, int>> SongView::selectedClipList() const {
    std::vector<std::pair<int, int>> out;
    if (selectedClipsN() > 0) {
        for (const auto& r : sel_)
            if (r.kind == timeline::ItemRef::Kind::Clip && r.row >= 0 && r.row < (int) rows_.size())
                if (const int at = clipIndexOfId(rows_[(size_t) r.row], r.key); at >= 0)
                    out.push_back({r.row, at});
        return out;
    }
    if (selClipRow_ >= 0 && selClip_ >= 0) out.push_back({selClipRow_, selClip_});
    return out;
}

void SongView::copySelectedClips() {
    const auto sel = selectedClipList();
    if (sel.empty()) return;
    int baseRow = INT_MAX, baseTick = INT_MAX, endTick = 0;
    for (const auto& [row, clip] : sel)
        for (const auto& ci : host().clips().list(rows_[(size_t) row]))
            if (ci.index == clip) {
                baseRow = std::min(baseRow, row);
                baseTick = std::min(baseTick, ci.startTick);
                endTick = std::max(endTick, ci.startTick + ci.lengthTicks);
            }
    if (baseRow == INT_MAX) return;
    clipboardRow_ = baseRow;
    clipboardTick_ = baseTick;
    clipboard_.clear();
    clipboardSpan_ = std::max(1, endTick - baseTick);
    for (const auto& [row, clip] : sel) {
        const auto& node = rows_[(size_t) row];
        for (const auto& ci : host().clips().list(node))
            if (ci.index == clip)
                clipboard_.push_back({row - baseRow, ci.startTick - baseTick,
                                      host().clips().copyClip(node, clip)});
    }
}

bool SongView::pasteClips(int atTick, int atRow) {
    if (clipboard_.empty()) return false;
    host().beginTransaction();
    host().pushUndo();
    sel_.clear();
    bool any = false;
    for (const auto& c : clipboard_) {
        const int row = atRow + c.row;
        if (row < 0 || row >= (int) rows_.size()) continue;
        const auto& node = rows_[(size_t) row];
        const int n = host().clips().pasteClip(node, c.data, std::max(0, atTick + c.startTick));
        if (n < 0) continue;
        any = true;
        for (const auto& ci : host().clips().list(node))
            if (ci.index == n) sel_.insert({timeline::ItemRef::Kind::Clip, row, ci.id});
    }
    host().endTransaction();
    if (any) rebuild();
    return any;
}

bool SongView::duplicateSelectedClips() {
    const auto sel = selectedClipList();
    if (sel.empty()) return false;
    copySelectedClips();
    int baseRow = INT_MAX, baseTick = INT_MAX;
    for (const auto& [row, clip] : sel)
        for (const auto& ci : host().clips().list(rows_[(size_t) row]))
            if (ci.index == clip) {
                baseRow = std::min(baseRow, row);
                baseTick = std::min(baseTick, ci.startTick);
            }
    if (baseRow == INT_MAX) return false;
    const int span = selectionSpanTicks();
    return pasteClips(baseTick + (span > 0 ? span : clipboardSpan_), baseRow);
}

}
