// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"
#include "gui/tracks/CutGuide.h"


#include <cstdio>
#include <cstdlib>

#include <cmath>

#include "gui/tracks/AutoPointPopup.h"
#include "gui/app/QwertyPiano.h"
#include "gui/common/Localisation.h"
#include "gui/tracks/ClipChrome.h"
#include "gui/tracks/TimelineTools.h"


namespace hum {

void SongView::mouseDown(const juce::MouseEvent& e) { mouseDownAt(paneEvent(e)); }
void SongView::mouseDrag(const juce::MouseEvent& e) { mouseDragAt(paneEvent(e)); }
void SongView::mouseUp(const juce::MouseEvent& e) { mouseUpAt(paneEvent(e)); }
void SongView::mouseMove(const juce::MouseEvent& e) { mouseMoveAt(paneEvent(e)); }
void SongView::mouseDoubleClick(const juce::MouseEvent& e) { mouseDoubleClickAt(paneEvent(e)); }

void SongView::mouseDownAt(const juce::MouseEvent& e) {
    traceSel("down", e);
    const auto p = e.getPosition();
    if (mouseDownAutoLane(e, p)) return;
    if (mouseDownBoxRow(e, p)) return;
    const int row = rowAt(p.y);
    if (row < 0) {
        if (e.mods.isPopupMenu() && p.x < kStripW && p.y >= headerH())
            showAddTrackMenu(paneToScreen(p));
        return;
    }
    if (p.x < kStripW) mouseDownHeader(e, row, p);
    else mouseDownBody(e, row, p);
}

void SongView::mouseUpAt(const juce::MouseEvent& e) {
    const bool gesture = drag_ != Drag::None || dragRow_ >= 0 || dragBox_ >= 0 || dragAutoPoint_ >= 0
                         || dragCurveIndex_ >= 0 || lineSlot_ >= 0 || repeatSrcId_ != 0 || !moveBase_.empty()
                         || !dragAutoNode_.empty();
    dragSync_.reset();
    traceSel("up", e);
    if (drag_ == Drag::BoxMove && dragBox_ >= 0 && std::abs(boxDragDelta_) > 1e-9) {
        host().automation().moveBox(dragBox_, boxDragDelta_);
        rebuild();
    }
    if ((drag_ == Drag::BoxTrimL || drag_ == Drag::BoxTrimR) && dragBox_ >= 0
        && std::abs(boxTrimL_) + std::abs(boxTrimR_) > 1e-9) {
        host().automation().trimBox(dragBox_, boxOrigS_ + boxTrimL_, boxOrigE_ + boxTrimR_);
        rebuild();
    }
    if (drag_ == Drag::Line && lineSlot_ >= 0)
        commitLine(dragAutoNode_, dragAutoParam_, lineBeat0_, lineVal0_, lineBeat1_, lineVal1_);
    if (drag_ == Drag::PointMarquee && !e.mouseWasDraggedSinceMouseDown()
        && ptMarquee_.getWidth() < 3 && ptMarquee_.getHeight() < 3 && !e.mods.isShiftDown())
        addPointAtClick(e, selPtSlot_);
    lineSlot_ = -1;
    pencilLast_ = -1.0;
    dragBox_ = -1;
    boxDragDelta_ = boxTrimL_ = boxTrimR_ = 0.0;
    clipMarquee_ = {};
    drag_ = Drag::None;
    moveBase_.clear();
    dragRow_ = dragClip_ = -1;
    dragAutoSlot_ = dragAutoPoint_ = -1;
    dragCurveIndex_ = -1;
    repeatSrcId_ = 0;
    repeatIds_.clear();
    dragAutoGrip_ = AutoGrip::Value;
    dragAutoNode_.clear(); dragAutoParam_.clear();
    if (gesture) repaintAll();
}

void SongView::mouseDoubleClickAt(const juce::MouseEvent& e) {
    traceSel("dbl", e);
    const auto p = e.getPosition();
    if (p.x < kStripW && p.y >= headerH()) {
        if (const int r = rowAt(p.y); r >= 0) {
            selectClip(r, -1);
            ctx_.nodeSelected(rows_[(size_t) r]);
        }
        return;
    }
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::BoxRow) {
        if (const int bx = boxAt(slots_[(size_t) s].track, p); bx >= 0 && !inBox())
            ctx_.openBoxDetail(host().automation().boxes()[(size_t) bx].organism);
        return;
    }
    if (const int s = trackslayout::slotAt(slots_, p.y);
        s >= 0 && slots_[(size_t) s].kind == trackslayout::Kind::AutoLane) {
        const auto& sl = slots_[(size_t) s];
        const auto& node = rows_[(size_t) sl.track];
        if (const int hit = autoPointAt(sl, node, sl.param, p); hit >= 0) {
            editAutoPoint(node, sl.param, hit, paneToScreen(p));
            return;
        }
        if (sl.laneKind != "double") ctx_.openAutomation(node, sl.param);
        return;
    }
    const int row = rowAt(p.y);
    if (row < 0 || p.x < kStripW) return;
    bool l = false, r = false;
    const int clip = clipAt(row, p, l, r);
    if (const auto h = rowClipHit(row, p); h == ClipHit::CurveL || h == ClipHit::CurveR) {
        const auto clips = host().clips().list(rows_[(size_t) row]);
        if (clip >= 0 && clip < (int) clips.size()) {
            const auto& ci = clips[(size_t) clip];
            host().pushUndo();
            host().clips().setFadeCurves(rows_[(size_t) row], clip,
                                        h == ClipHit::CurveL ? 0.0 : ci.fadeInCurve,
                                        h == ClipHit::CurveR ? 0.0 : ci.fadeOutCurve);
            repaintAll();
        }
        return;
    }
    if (const int bx = boxAt(row, p); clip < 0 && bx >= 0) {
        ctx_.openBoxDetail(host().automation().boxes()[(size_t) bx].organism);
        return;
    }
    if (clip >= 0) {
        const auto clips = host().clips().list(rows_[(size_t) row]);
        if (clip < (int) clips.size()) {
            const auto& ci = clips[(size_t) clip];
            if (ci.isAudio) {
                ctx_.enterClip(rows_[(size_t) row], ci.id);
                return;
            }
            if (ci.isVideo || ci.isCompound) return;
        }
        ctx_.enterTrack(rows_[(size_t) row]);
    }
}

void SongView::mouseMoveAt(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    const auto was = hover_;
    hover_ = p;
    const int row = rowAt(p.y);
    const Tool tool = effectiveTool();
    if (tool == Tool::Scissors) cutguide::repaintMove(*this, view_, ctx_, was.x, p.x);
    if (const int s = trackslayout::slotAt(slots_, p.y); s >= 0 && p.x >= kStripW) {
        const auto& sl = slots_[(size_t) s];
        if (sl.kind == trackslayout::Kind::AutoLane) {
            const int hit = autoPointAt(sl, rows_[(size_t) sl.track], sl.param, p);
            if (hit != hoverPt_ || s != hoverPtSlot_) {
                hoverPtSlot_ = s; hoverPt_ = hit;
                repaintPane({kStripW, sl.y, getWidth() - kStripW, sl.h});
            }
            if (tool != Tool::Pointer) { setMouseCursor(timelinechrome::toolCursor(tool)); return; }
            setMouseCursor(hit >= 0 ? juce::MouseCursor::DraggingHandCursor
                                    : juce::MouseCursor::NormalCursor);
            return;
        }
        if (sl.kind == trackslayout::Kind::BoxRow) {
            bool bl = false, br = false;
            boxAt(sl.track, p, bl, br);
            setMouseCursor(bl || br ? juce::MouseCursor::LeftRightResizeCursor
                                    : juce::MouseCursor::NormalCursor);
            return;
        }
    }
    if (hoverPt_ >= 0 && hoverPtSlot_ >= 0 && hoverPtSlot_ < (int) slots_.size()) {
        const auto& hs = slots_[(size_t) hoverPtSlot_];
        repaintPane({kStripW, hs.y, getWidth() - kStripW, hs.h});
        hoverPtSlot_ = hoverPt_ = -1;
    }
    if (tool != Tool::Pointer && row >= 0 && p.x >= kStripW) {
        setMouseCursor(timelinechrome::toolCursor(tool));
        return;
    }
    bool l = false, r = false;
    if (row >= 0 && p.x >= kStripW) {
        if (const int c = clipAt(row, p, l, r); c < 0) boxAt(row, p, l, r);
        else if (const auto clips = host().clips().list(rows_[(size_t) row]);
                 c < (int) clips.size()) {
            const auto cb = clipBounds(row, clips[(size_t) c]);
            const auto& hc = clips[(size_t) c];
            if (const auto fg = hc.hasMedia()
                    ? timelinechrome::fadeGripAt(cb, p, kFadeGrip, hc.fadeInTicks,
                                                 hc.fadeOutTicks, hc.lengthTicks)
                    : timelinechrome::FadeGrip::None;
                fg != timelinechrome::FadeGrip::None) {
                setMouseCursor(fg == timelinechrome::FadeGrip::Left
                                   ? juce::MouseCursor::TopLeftCornerResizeCursor
                                   : juce::MouseCursor::TopRightCornerResizeCursor);
                return;
            }
            if (hc.hasMedia()
                && timelinechrome::fadeCurveGripAt(cb, p, kFadeGrip, hc.fadeInTicks,
                                                   hc.fadeOutTicks, hc.lengthTicks,
                                                   hc.fadeInCurve, hc.fadeOutCurve)
                       != timelinechrome::FadeGrip::None) {
                setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
                return;
            }
            if (!clips[(size_t) c].looped && overRepeatGrip(cb, p)) {
                setMouseCursor(juce::MouseCursor::CopyingCursor);
                return;
            }
        }
    }
    setMouseCursor(l || r ? juce::MouseCursor::LeftRightResizeCursor
                          : juce::MouseCursor::NormalCursor);
}

juce::String SongView::getTooltip() {
    const auto p = hover_;
    const auto handleTip = [](ClipHit h) -> juce::String {
        switch (h) {
            case ClipHit::FadeL:  return tr("tracks-pane-roll.fade-in-drag-to-set", "Fade in - drag to set how long it takes");
            case ClipHit::FadeR:  return tr("tracks-pane-roll.fade-out-drag-to-set", "Fade out - drag to set how long it takes");
            case ClipHit::CurveL:
            case ClipHit::CurveR:
                return tr("tracks-pane-roll.fade-shape-drag-up-or", "Fade shape - drag up or down to bend it, double-click to straighten");
            case ClipHit::EdgeL:  return tr("tracks-pane-roll.drag-to-trim-the-start-mac", "Drag to trim the start - Cmd-drag to stretch");
            case ClipHit::EdgeR:  return tr("tracks-pane-roll.drag-to-trim-the-end-mac", "Drag to trim the end - Cmd-drag to stretch");
            default:              return {};
        }
    };
    if (p.x >= kStripW && effectiveTool() == Tool::Pointer) {
        if (const int row = rowAt(p.y); row >= 0) {
            const auto h = rowClipHit(row, p);
            if (const auto t = handleTip(h); t.isNotEmpty()) return t;
            if (h == ClipHit::Body) {
                bool l = false, r = false;
                const int c = clipAt(row, p, l, r);
                const auto clips = host().clips().list(rows_[(size_t) row]);
                if (c >= 0 && c < (int) clips.size() && !clips[(size_t) c].looped
                    && overRepeatGrip(clipBounds(row, clips[(size_t) c]), p))
                    return tr("tracks-pane-roll.drag-to-repeat-the-clip", "Drag to repeat the clip");
                return tr("tracks-pane-roll.drag-to-move-cmd-drag-mac", "Drag to move - Cmd-drag to duplicate, double-click to edit");
            }
        }
    }
    if (!inBox() && p.x < kStripW && p.y >= headerH()) {
        if (const int row = rowAt(p.y); row >= 0) {
            if (muteBox(row).contains(p)) return "Mute";
            if (soloBox(row).contains(p)) return "Solo";
            if (recBox(row).contains(p)) return "Arm";
            if (heldBox(row).contains(p)) return tr("tracks-pane-roll.a-hand-is-holding-this", "A hand is holding this lane - click to let go");
            if (foldBox(row).contains(p)) return tr("tracks-pane-roll.show-what-is-folded-under", "Show what is folded under this track");
        }
    }
    return {};
}

}
