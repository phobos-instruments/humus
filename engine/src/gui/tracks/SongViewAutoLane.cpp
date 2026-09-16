// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/tracks/SongView.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "gui/tracks/AutoPointPopup.h"
#include "gui/common/Localisation.h"

namespace hum {

bool SongView::mouseDownAutoLane(const juce::MouseEvent& e, juce::Point<int> p) {
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::PodHeader) {
        const auto& pod = slots_[(size_t) s].param;
        if (p.x < kStripW) {
            if (!collapsedPods_.insert(pod).second) collapsedPods_.erase(pod);
            rebuildSlots();
            repaintAll();
        }
        return true;
    }
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::AutoLane) {
        const auto& sl = slots_[(size_t) s];
        const auto& lnode = rows_[(size_t) sl.track];
        if (heldLaneBox(sl).contains(p) && host().automation().isHeld(lnode, sl.param)) {
            host().automation().release(lnode, sl.param);
            repaintAll();
            return true;
        }
        if (p.x >= kStripW && !e.mods.isPopupMenu()) {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            if (tryBeginPointSelection(e, s, hit)) return true;
            if (hit < 0 && !selPts_.empty()) clearPointSelection();
        }
        bool muted = false;
        std::string kind = "double";
        if (const auto* cm = host().model().byName(lnode))
            for (const auto& l : cm->automation)
                if (l.propertyName == sl.param)
                    { muted = l.mute; kind = l.kind; break; }
        if (e.mods.isPopupMenu()) {
            const int hit = (kind == "double" && p.x >= kStripW)
                                ? autoPointAt(sl, lnode, sl.param, p) : -1;
            if (hit >= 0) {
                host().pushUndo();
                host().automation().deletePoint(lnode, sl.param, hit);
                repaintAll();
                return true;
            }
            juce::PopupMenu m;
            m.addItem(1, tr("tracks-pane-input.clear-lane-points", "Clear Lane Points"));
            m.addItem(2, tr("tracks-pane-input.delete-lane", "Delete Lane"));
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, node = lnode, param = sl.param](int r) {
                if (r == 1) host().automation().clearLane(node, param);
                else if (r == 2) host().automation().remove(node, param);
                if (r > 0) rebuild();
            });
            return true;
        }
        if (p.x < kStripW) {
            const int by = sl.y + (sl.h - 12) / 2;
            if (juce::Rectangle<int>(kStripW - 54, by, 22, 12).contains(p))
                host().automation().setLaneMute(lnode, sl.param, !muted);
            repaintAll();
        } else if (kind == "double") {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            const auto tool = effectiveTool();
            if (tool == Tool::Eraser) {
                if (hit >= 0) { host().pushUndo(); host().automation().deletePoint(lnode, sl.param, hit); }
                repaintAll();
                return true;
            }
            if (tool == Tool::Draw || tool == Tool::Line) {
                host().pushUndo();
                const auto [lo, hi] = laneRange(lnode, sl.param);
                lineSlot_ = s;
                dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
                lineBeat0_ = lineBeat1_ = tool == Tool::Line || e.mods.isShiftDown()
                                              ? snapBeats(std::max(0.0, xToBeat((float) p.x)), e.mods.isAltDown())
                                              : std::max(0.0, xToBeat((float) p.x));
                lineVal0_ = lineVal1_ = laneValueAtY(sl, p.y, lo, hi);
                if (tool == Tool::Line || e.mods.isShiftDown()) drag_ = Drag::Line;
                else {
                    drag_ = Drag::Pencil;
                    pencilLast_ = -1.0;
                    pencilStep(lnode, sl.param, lineBeat0_, lineVal0_);
                }
                repaintAll();
                return true;
            }
            if (e.mods.isAltDown()) {
                if (hit >= 0) { host().pushUndo(); host().automation().deletePoint(lnode, sl.param, hit); }
                repaintAll();
                return true;
            }
            if (hit < 0) {
                if (const int seg = segmentAt(sl, lnode, sl.param, p); seg >= 0) {
                    host().pushUndo();
                    drag_ = Drag::Curve;
                    dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
                    dragCurveIndex_ = seg;
                    dragCurve0_ = host().automation().curveAt(lnode, sl.param, seg);
                    dragCurveY0_ = p.y;
                    return true;
                }
            }
            host().pushUndo();
            const auto [lo, hi] = laneRange(lnode, sl.param);
            const double beat = snapBeats(std::max(0.0, xToBeat((float) p.x)), e.mods.isAltDown());
            int idx = hit;
            if (idx < 0) {
                host().automation().addPoint(lnode, sl.param, beat, laneValueAtY(sl, p.y, lo, hi));
                idx = pointIndexAtBeat(lnode, sl.param, beat);
            }
            dragAutoSlot_ = s; dragAutoPoint_ = idx;
            dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
            dragAutoGrip_ = AutoGrip::Value;
            repaintAll();
        } else if (kind == "trigger") {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            if (e.mods.isAltDown()) {
                if (hit >= 0) { host().pushUndo(); host().automation().deletePoint(lnode, sl.param, hit); }
                repaintAll();
                return true;
            }
            host().pushUndo();
            const double beat = snapBeats(std::max(0.0, xToBeat((float) p.x)), e.mods.isAltDown());
            int idx = hit;
            if (idx < 0) {
                host().automation().addTriggerPoint(lnode, sl.param, beat);
                idx = pointIndexAtBeat(lnode, sl.param, beat);
            }
            dragAutoSlot_ = s; dragAutoPoint_ = idx;
            dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
            dragAutoGrip_ = AutoGrip::TriggerTime;
            repaintAll();
        } else if (kind == "range") {
            const int hit = autoPointAt(sl, lnode, sl.param, p);
            if (e.mods.isAltDown()) {
                if (hit >= 0) { host().pushUndo(); host().automation().deletePoint(lnode, sl.param, hit); }
                repaintAll();
                return true;
            }
            host().pushUndo();
            const auto [lo, hi] = laneRange(lnode, sl.param);
            const double beat = snapBeats(std::max(0.0, xToBeat((float) p.x)), e.mods.isAltDown());
            const double v = laneValueAtY(sl, p.y, lo, hi);
            int idx = hit;
            if (idx < 0) {
                host().automation().addRangePoint(lnode, sl.param, beat, v, v);
                idx = pointIndexAtBeat(lnode, sl.param, beat);
            }
            dragAutoSlot_ = s; dragAutoPoint_ = idx;
            dragAutoNode_ = lnode; dragAutoParam_ = sl.param;
            dragAutoGrip_ = e.mods.isShiftDown() ? AutoGrip::RangeBoth
                                                 : nearerRangeEdge(sl, lnode, sl.param, idx, p);
            repaintAll();
        }
        return true;
    }
    return false;
}

void SongView::addPointAtClick(const juce::MouseEvent& e, int slot) {
    if (slot < 0 || slot >= (int) slots_.size()) return;
    const auto& sl = slots_[(size_t) slot];
    if (sl.kind != trackslayout::Kind::AutoLane || e.x < kStripW) return;
    const auto& node = rows_[(size_t) sl.track];
    const auto* lane = laneOfSlot(sl);
    if (lane == nullptr) return;
    const double beat = snapBeats(std::max(0.0, xToBeat((float) e.x)), e.mods.isAltDown());
    const auto [lo, hi] = laneRange(node, sl.param);
    const double v = laneValueAtY(sl, e.y, lo, hi);
    host().pushUndo();
    if (lane->kind == "trigger") host().automation().addTriggerPoint(node, sl.param, beat);
    else if (lane->kind == "range") host().automation().addRangePoint(node, sl.param, beat, v, v);
    else host().automation().addPoint(node, sl.param, beat, v);
    clearPointSelection();
}

int SongView::pointIndexAtBeat(const std::string& node, const std::string& param,
                                 double beat) const {
    const auto* cm = host().model().byName(node);
    if (cm == nullptr) return -1;
    for (const auto& l : cm->automation) {
        if (l.propertyName != param) continue;
        int best = -1;
        double bestD = 1.0e-6;
        for (int i = 0; i < (int) l.points.size(); ++i)
            if (const double d = std::abs(l.points[(size_t) i].beat - beat); d < bestD) { bestD = d; best = i; }
        return best;
    }
    return -1;
}

void SongView::editAutoPoint(const std::string& node, const std::string& param, int index,
                               juce::Point<int> screenAt) {
    const auto* cm = host().model().byName(node);
    if (cm == nullptr) return;
    const hum::AutomationLane* lane = nullptr;
    for (const auto& l : cm->automation)
        if (l.propertyName == param) { lane = &l; break; }
    if (lane == nullptr || index < 0 || index >= (int) lane->points.size()) return;
    const auto& pt = lane->points[(size_t) index];
    const auto [lo, hi] = laneRange(node, param);
    AutoPointPopup::Point point{lane->kind, pt.beat, pt.value, pt.valueMax};
    AutoPointPopup::show(
        {screenAt.x - 4, screenAt.y - 4, 8, 8}, juce::String(param), point, lo, hi,
        paramUnit(host(), node, param), host().automation().meterMap(),
        [this, node, param, index, kind = lane->kind](AutoPointPopup::Point out) {
            host().pushUndo();
            if (kind == "range")
                host().automation().moveRangePoint(node, param, index, out.beat, out.value, out.valueMax);
            else
                host().automation().movePoint(node, param, index, out.beat,
                                             kind == "trigger" ? 0.0 : out.value);
            repaintAll();
        });
}

}
