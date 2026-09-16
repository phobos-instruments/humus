// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "gui/tracks/AutoPointPopup.h"
#include "gui/common/Localisation.h"
#include "gui/tracks/ClipChrome.h"

namespace hum {

void SongView::showBoxMenu(int bx, juce::Point<int> sp) {
    selBox_ = bx;
    juce::PopupMenu m;
    m.addItem(1, tr("tracks-pane-input.duplicate-after", "Duplicate After"));
    m.addItem(2, tr("tracks-pane-input.delete", "Delete"));
    m.addItem(3, tr("tracks-pane-input.merge", "Merge"), sel_.size() > 1);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                    [this, bx](int r) {
        const auto& boxes = host().automation().boxes();
        if (r == 3) { mergeSelection(); return; }
        if (bx >= (int) boxes.size()) return;
        if (r == 1)
            selBox_ = host().automation().duplicateBox(bx, boxes[(size_t) bx].endBeat);
        else if (r == 2) {
            host().automation().deleteBox(bx);
            selBox_ = -1;
        }
        rebuild();
    });
    repaintAll();
}

void SongView::beginBoxDrag(int row, int bx, bool leftEdge, bool rightEdge, juce::Point<int> p) {
    clearClipSel();
    selBox_ = bx;
    dragBox_ = bx;
    dragRow_ = row;
    boxAnchor_ = xToBeat((float) p.x);
    boxDragDelta_ = boxTrimL_ = boxTrimR_ = 0.0;
    boxOrigS_ = host().automation().boxes()[(size_t) bx].startBeat;
    boxOrigE_ = host().automation().boxes()[(size_t) bx].endBeat;
    drag_ = leftEdge ? Drag::BoxTrimL : rightEdge ? Drag::BoxTrimR : Drag::BoxMove;
    repaintAll();
}

bool SongView::mouseDownBoxRow(const juce::MouseEvent& e, juce::Point<int> p) {
    const int s = trackslayout::slotAt(slots_, p.y);
    if (s < 0 || slots_[(size_t) s].kind != trackslayout::Kind::BoxRow) return false;
    const auto& sl = slots_[(size_t) s];
    const int row = sl.track;
    if (p.x < kStripW) {
        const auto& node = rows_[(size_t) row];
        if (!expanded_.insert(node).second) expanded_.erase(node);
        rebuildSlots();
        repaintAll();
        return true;
    }
    bool bl = false, br = false;
    const int bx = boxAt(row, p, bl, br);
    if (e.mods.isPopupMenu()) {
        if (bx >= 0) showBoxMenu(bx, e.getScreenPosition());
        return true;
    }
    if (e.mods.isShiftDown()) {
        if (bx >= 0) toggleSelected(boxRef(row, bx));
        repaintAll();
        return true;
    }
    if (applyToolAt(row, p, e.mods.isAltDown())) return true;
    if (bx >= 0) beginBoxDrag(row, bx, bl, br, p);
    else {
        selBox_ = -1;
        clearSelection();
        marqueeAnchor_ = p;
        clipMarquee_ = juce::Rectangle<int>(p, p);
        drag_ = Drag::ClipMarquee;
        repaintAll();
    }
    return true;
}

void SongView::mouseDownBody(const juce::MouseEvent& e, int row, juce::Point<int> p) {
    const auto& node = rows_[(size_t) row];
    bool leftEdge = false, rightEdge = false;
    const int clip = clipAt(row, p, leftEdge, rightEdge);
    const int tick = std::max(0, xToTick((float) p.x));

    if (e.mods.isPopupMenu()) {
        if (const int bx = boxAt(row, p); clip < 0 && bx >= 0) {
            showBoxMenu(bx, e.getScreenPosition());
            return;
        }
        if (clip >= 0) {
            if (sel_.size() <= 1 && !selected(clipRef(row, clip))) {
                clearSelection();
                sel_.insert(clipRef(row, clip));
            }
            selectClip(row, clip);
            repaintAll();
            showClipMenu(row, clip, e.getScreenPosition(), tick);
        } else {
            juce::PopupMenu m;
            const bool audio = host().nodeRecordsAudio(node);
            if (audio) m.addItem(1, tr("tracks-pane-input.import-audio-file", "Import Audio File..."));
            m.addItem(2, tr("tracks-pane-input.cut-at-playhead", "Cut at Playhead"), clipsUnderPlayhead() > 0);
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options()
                                .withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, node, tick, alt = e.mods.isAltDown()](int r) {
                if (r == 2) { cutAtPlayhead(); return; }
                if (r != 1) return;
                const int start = (int) std::llround(
                    snapBeats(tick / (double) Pattern::kTicksPerBeat, alt)
                    * Pattern::kTicksPerBeat);
                importAudioInto(node, start);
            });
        }
        return;
    }

    if (e.mods.isShiftDown()) {
        if (sel_.empty() && selClipRow_ >= 0 && selClip_ >= 0)
            sel_.insert(clipRef(selClipRow_, selClip_));
        if (clip >= 0) toggleSelected(clipRef(row, clip));
        else if (const int bx = boxAt(row, p); bx >= 0) toggleSelected(boxRef(row, bx));
        syncTimeSelection();
        repaintAll();
        return;
    }

    if (applyToolAt(row, p, e.mods.isAltDown())) return;

    if (bool bl = false, br = false; clip < 0)
        if (const int bx = boxAt(row, p, bl, br); bx >= 0) {
            beginBoxDrag(row, bx, bl, br, p);
            return;
        }
    selBox_ = -1;

    dragRow_ = row;
    if (clip >= 0 && !selected(clipRef(row, clip))) { clearSelection(); sel_.insert(clipRef(row, clip)); }
    if (clip >= 0 && effectiveTool() == Tool::Draw) {
        selectClip(row, clip);
        repaintAll();
        return;
    }
    if (clip >= 0) {
        host().pushUndo();
        const auto clips = host().clips().list(node);
        const auto& ci = clips[(size_t) clip];
        dragOriginTick_ = ci.startTick;
        dragOriginNode_ = node;
        dragDuplicated_ = false;
        if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
            const int made = beginClipMove(row, clip, true);
            if (made < 0) { repaintAll(); return; }
            dragClip_ = made;
            dragDuplicated_ = true;
            selectClip(row, made);
            drag_ = Drag::ClipMove;
            dragGrabTicks_ = tick - ci.startTick;
        } else if (const auto fg = ci.hasMedia()
                       ? timelinechrome::fadeGripAt(clipBounds(row, ci), p, kFadeGrip,
                                                    ci.fadeInTicks, ci.fadeOutTicks,
                                                    ci.lengthTicks)
                       : timelinechrome::FadeGrip::None;
                   fg != timelinechrome::FadeGrip::None) {
            drag_ = fg == timelinechrome::FadeGrip::Left ? Drag::ClipFadeL : Drag::ClipFadeR;
            dragClip_ = clip;
            selectClip(row, clip);
        } else if (const auto cg = ci.hasMedia()
                       ? timelinechrome::fadeCurveGripAt(
                             clipBounds(row, ci), p, kFadeGrip, ci.fadeInTicks,
                             ci.fadeOutTicks, ci.lengthTicks, ci.fadeInCurve, ci.fadeOutCurve)
                       : timelinechrome::FadeGrip::None;
                   cg != timelinechrome::FadeGrip::None) {
            drag_ = cg == timelinechrome::FadeGrip::Left ? Drag::ClipFadeCurveL
                                                        : Drag::ClipFadeCurveR;
            dragClip_ = clip;
            dragCurveY0_ = p.y;
            dragCurve0_ = cg == timelinechrome::FadeGrip::Left ? ci.fadeInCurve : ci.fadeOutCurve;
            host().pushUndo();
            selectClip(row, clip);
        } else if (!ci.looped && overRepeatGrip(clipBounds(row, ci), p)) {
            drag_ = Drag::ClipRepeat;
            dragClip_ = clip;
            repeatSrcId_ = ci.id;
            repeatStart_ = ci.startTick;
            repeatLen_ = std::max(1, ci.lengthTicks);
            repeatIds_.clear();
            selectClip(row, clip);
        } else if (leftEdge) {
            drag_ = Drag::ClipResizeL;
            dragClip_ = clip;
            selectClip(row, clip);
        } else if (rightEdge) {
            drag_ = Drag::ClipResizeR;
            dragClip_ = clip;
            selectClip(row, clip);
        } else {
            drag_ = Drag::ClipMove;
            dragClip_ = clip;
            dragGrabTicks_ = tick - ci.startTick;
            selectClip(row, clip);
            beginClipMove(row, clip, false);
        }
        syncTimeSelection();
    } else {
        clearClipSel();
        clearSelection();
        if (effectiveTool() != Tool::Draw) {
            marqueeAnchor_ = p;
            clipMarquee_ = juce::Rectangle<int>(p, p);
            drag_ = Drag::ClipMarquee;
            repaintAll();
            return;
        }
        if (host().nodeRecordsAudio(node) || host().nodeArrangesVideo(node)
            || arrangeable_.count(node) == 0) {
            repaintAll();
            return;
        }
        host().pushUndo();
        const int start = (int) std::llround(
            snapBeats(tick / (double) Pattern::kTicksPerBeat, e.mods.isAltDown())
            * Pattern::kTicksPerBeat);
        dragClip_ = host().clips().add(node, start, barTicks());
        drag_ = Drag::ClipCreate;
        selectClip(row, dragClip_);
    }
    repaintAll();
}

}
