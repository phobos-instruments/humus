// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/ClipEditorView.h"
#include "gui/tracks/CutGuide.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "gui/common/Localisation.h"
#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/tracks/ClipChrome.h"
#include "gui/tracks/TimelineTools.h"
#include "gui/tracks/TracksGeometry.h"
#include "gui/tracks/WaveformCache.h"
#include "io/PatchDocument.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
using Tool = noteedit::Tool;
}

ClipEditorView::SelectionWatch::~SelectionWatch() {
    const auto& now = v_.view_.sel;
    const bool wasShown = was_.active && was_.to > was_.from;
    const bool isShown = now.active && now.to > now.from;
    if (wasShown == isShown && (!isShown || (was_.from == now.from && was_.to == now.to))) return;
    double a = isShown ? now.from : was_.from, b = isShown ? now.to : was_.to;
    if (wasShown && isShown) { a = std::min(a, was_.from); b = std::max(b, was_.to); }
    v_.ctx_.beatsChanged(a, b);
    v_.repaint(v_.ribbonBounds() - v_.getPosition());
}

void ClipEditorView::open(const std::string& node, int id) {
    node_ = node;
    id_ = id;
    drag_ = Drag::None;
    sync_.reset();
    setVisible(true);
}

void ClipEditorView::close() {
    node_.clear();
    id_ = -1;
    drag_ = Drag::None;
    sync_.reset();
    setVisible(false);
}

float ClipEditorView::beatToX(double beat) const { return view_.beatToX(beat, tracksgeo::kStripW); }

double ClipEditorView::xToBeat(float x) const { return view_.xToBeat(x, tracksgeo::kStripW); }

float ClipEditorView::tickToX(int tick) const { return view_.tickToX(tick, tracksgeo::kStripW); }

void ClipEditorView::mouseDown(const juce::MouseEvent& e) {
    SelectionWatch watch(*this);
    mouseDownClip(e, toPane(e.getPosition()));
}

void ClipEditorView::mouseDrag(const juce::MouseEvent& e) {
    SelectionWatch watch(*this);
    if (!sync_ && drag_ != Drag::None) sync_.emplace(host());
    ctx_.edgeScroll(toPane(e.getPosition()));
    mouseDragClip(e);
}

void ClipEditorView::mouseUp(const juce::MouseEvent&) {
    SelectionWatch watch(*this);
    sync_.reset();
    mouseUpClip();
}

void ClipEditorView::mouseMove(const juce::MouseEvent& e) {
    const auto p = toPane(e.getPosition());
    setMouseCursor(clipEditorCursor(p));
    const auto was = hover_;
    hover_ = p;
    if (ctx_.effectiveTool() == Tool::Scissors) cutguide::repaintMove(*this, view_, ctx_, was.x, p.x);
}

void ClipEditorView::repaintCutGuide() { cutguide::repaintMove(*this, view_, ctx_, hover_.x, hover_.x); }

void ClipEditorView::mouseDoubleClick(const juce::MouseEvent& e) {
    const auto h = clipEditorHit(toPane(e.getPosition()));
    if (h == Hit::CurveL || h == Hit::CurveR) straightenFade(h == Hit::CurveL);
}

bool ClipEditorView::keyPressed(const juce::KeyPress& k) {
    SelectionWatch watch(*this);
    return keyPressedClip(k);
}

juce::String ClipEditorView::getTooltip() {
    if (ctx_.effectiveTool() != Tool::Pointer) return {};
    switch (clipEditorHit(hover_)) {
        case Hit::FadeL:  return tr("tracks-pane-roll.fade-in-drag-to-set", "Fade in - drag to set how long it takes");
        case Hit::FadeR:  return tr("tracks-pane-roll.fade-out-drag-to-set", "Fade out - drag to set how long it takes");
        case Hit::CurveL:
        case Hit::CurveR:
            return tr("tracks-pane-roll.fade-shape-drag-up-or", "Fade shape - drag up or down to bend it, double-click to straighten");
        case Hit::EdgeL:  return tr("tracks-pane-roll.drag-to-trim-the-start-mac", "Drag to trim the start - Cmd-drag to stretch");
        case Hit::EdgeR:  return tr("tracks-pane-roll.drag-to-trim-the-end-mac", "Drag to trim the end - Cmd-drag to stretch");
        case Hit::Body:   return tr("tracks-pane-roll.drag-to-select-a-range", "Drag to select a range - Alt-drag to slip the audio");
        case Hit::None:   break;
    }
    return {};
}

int ClipEditorView::clipOrdinal() const {
    return id_ > 0 ? ctx_.clipIndexOfId(node_, id_) : -1;
}

bool ClipEditorView::clipInfo(ClipEditor::ClipInfo& ci) const {
    const int c = clipOrdinal();
    if (c < 0) return false;
    const auto clips = host().clips().list(node_);
    if (c >= (int) clips.size()) return false;
    ci = clips[(size_t) c];
    return true;
}

double ClipEditorView::samplesPerBeat() const {
    return (host().tempo() > 0.0 ? kSecondsPerMinute / host().tempo() : 0.5) * host().sampleRate();
}

juce::Rectangle<int> ClipEditorView::clipField() const {
    const int top = tracksgeo::headerH() + tracksgeo::kChipH;
    return {tracksgeo::kStripW, top, std::max(0, getWidth() - tracksgeo::kStripW),
            std::max(0, getBottom() - tracksgeo::kClipRibbonH - top)};
}

ClipEditorView::Hit ClipEditorView::clipEditorHit(juce::Point<int> p) const {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return Hit::None;
    const auto box = clipBox();
    if (!clipField().contains(p)) return Hit::None;
    using timelinechrome::FadeGrip;
    if (ci.hasMedia() && box.contains(p)) {
        const auto fg = timelinechrome::fadeGripAt(box, p, tracksgeo::kFadeGrip * 2, ci.fadeInTicks,
                                                   ci.fadeOutTicks, ci.lengthTicks);
        if (fg == FadeGrip::Left) return Hit::FadeL;
        if (fg == FadeGrip::Right) return Hit::FadeR;
        const auto cg = timelinechrome::fadeCurveGripAt(box, p, tracksgeo::kFadeGrip, ci.fadeInTicks,
                                                        ci.fadeOutTicks, ci.lengthTicks,
                                                        ci.fadeInCurve, ci.fadeOutCurve);
        if (cg == FadeGrip::Left) return Hit::CurveL;
        if (cg == FadeGrip::Right) return Hit::CurveR;
    }
    if (std::abs(p.x - box.getX()) <= tracksgeo::kClipEdgeGrab) return Hit::EdgeL;
    if (std::abs(p.x - box.getRight()) <= tracksgeo::kClipEdgeGrab) return Hit::EdgeR;
    return box.contains(p) ? Hit::Body : Hit::None;
}

juce::MouseCursor ClipEditorView::clipEditorCursor(juce::Point<int> p) const {
    if (ctx_.effectiveTool() != Tool::Pointer) return timelinechrome::toolCursor(ctx_.effectiveTool());
    switch (clipEditorHit(p)) {
        case Hit::EdgeL:
        case Hit::EdgeR:  return juce::MouseCursor::LeftRightResizeCursor;
        case Hit::FadeL:  return juce::MouseCursor::TopLeftCornerResizeCursor;
        case Hit::FadeR:  return juce::MouseCursor::TopRightCornerResizeCursor;
        case Hit::CurveL:
        case Hit::CurveR: return juce::MouseCursor::UpDownResizeCursor;
        case Hit::Body:   return juce::MouseCursor::IBeamCursor;
        case Hit::None:   break;
    }
    return juce::MouseCursor::NormalCursor;
}

juce::Rectangle<int> ClipEditorView::ribbonBounds() const {
    const auto f = clipField();
    return {tracksgeo::kStripW, f.getBottom(), getWidth() - tracksgeo::kStripW, getBottom() - f.getBottom()};
}

juce::Rectangle<int> ClipEditorView::clipBox() const {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return {};
    const auto f = clipField();
    const int x0 = (int) std::floor(tickToX(ci.startTick));
    const int x1 = (int) std::ceil(tickToX(ci.startTick + ci.lengthTicks));
    return {x0, f.getY(), std::max(1, x1 - x0), f.getHeight()};
}

bool ClipEditorView::clipSelectionTicks(int& from, int& to) const {
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci) || !view_.sel.active || view_.sel.to <= view_.sel.from) return false;
    from = std::max(ci.startTick, (int) std::llround(view_.sel.from * Pattern::kTicksPerBeat));
    to = std::min(ci.startTick + ci.lengthTicks,
                  (int) std::llround(view_.sel.to * Pattern::kTicksPerBeat));
    return to > from;
}

bool ClipEditorView::mouseDownClip(const juce::MouseEvent& e, juce::Point<int> p) {
    if (id_ <= 0) return false;
    const auto f = clipField();
    if (!f.contains(p)) return p.y >= tracksgeo::headerH();
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return true;
    if (e.mods.isPopupMenu()) { showClipDetailMenu(e.getScreenPosition()); return true; }

    const auto box = clipBox();
    const double beat = std::max(0.0, xToBeat((float) p.x));
    start0_ = ci.startTick;
    len0_ = ci.lengthTicks;
    offset0_ = ci.audioOffset;
    anchorBeat_ = beat;

    if (ctx_.effectiveTool() == Tool::Scissors) {
        splitClipAt((int) std::llround(ctx_.snapBeats(beat, e.mods.isAltDown()) * Pattern::kTicksPerBeat));
        return true;
    }
    const bool nearL = std::abs(p.x - box.getX()) <= tracksgeo::kClipEdgeGrab;
    const bool nearR = std::abs(p.x - box.getRight()) <= tracksgeo::kClipEdgeGrab;
    const auto fg = box.contains(p)
                        ? timelinechrome::fadeGripAt(box, p, tracksgeo::kFadeGrip * 2, ci.fadeInTicks,
                                                     ci.fadeOutTicks, ci.lengthTicks)
                        : timelinechrome::FadeGrip::None;
    const auto cg = box.contains(p)
                        ? timelinechrome::fadeCurveGripAt(box, p, tracksgeo::kFadeGrip, ci.fadeInTicks,
                                                          ci.fadeOutTicks, ci.lengthTicks,
                                                          ci.fadeInCurve, ci.fadeOutCurve)
                        : timelinechrome::FadeGrip::None;
    if (fg == timelinechrome::FadeGrip::Left) drag_ = Drag::FadeL;
    else if (fg == timelinechrome::FadeGrip::Right) drag_ = Drag::FadeR;
    else if (cg != timelinechrome::FadeGrip::None) {
        drag_ = cg == timelinechrome::FadeGrip::Left ? Drag::CurveL : Drag::CurveR;
        curveY0_ = p.y;
        curve0_ = cg == timelinechrome::FadeGrip::Left ? ci.fadeInCurve : ci.fadeOutCurve;
    }
    else if (nearL) drag_ = e.mods.isCommandDown() ? Drag::StretchL : Drag::TrimL;
    else if (nearR) drag_ = e.mods.isCommandDown() ? Drag::StretchR : Drag::TrimR;
    else if (e.mods.isAltDown() && box.contains(p)) drag_ = Drag::Slip;
    else drag_ = Drag::Pending;
    if (drag_ != Drag::Pending) host().pushUndo();
    return true;
}

void ClipEditorView::mouseDragClip(const juce::MouseEvent& e) {
    const auto pp = toPane(e.getPosition());
    if (drag_ == Drag::None) return;
    const double beat = std::max(0.0, xToBeat((float) pp.x));
    const double snapped = ctx_.snapBeats(beat, e.mods.isAltDown());
    const int c = clipOrdinal();
    if (c < 0) return;
    switch (drag_) {
        case Drag::Pending:
            if (std::abs(e.getDistanceFromDragStartX()) < 3) return;
            drag_ = Drag::Select;
            view_.sel.anchor = ctx_.snapBeats(anchorBeat_, e.mods.isAltDown());
            view_.sel.active = true;
            [[fallthrough]];
        case Drag::Select:
            view_.sel.from = std::min(view_.sel.anchor, snapped);
            view_.sel.to = std::max(view_.sel.anchor, snapped);
            break;
        case Drag::TrimL: {
            const auto ci = host().clips().list(node_)[(size_t) c];
            const int floorTick = ci.hasMedia() && !ci.audioReverse
                ? start0_ - (int) std::llround((double) offset0_ * Pattern::kTicksPerBeat / samplesPerBeat())
                : 0;
            const int end = start0_ + len0_;
            const int t = juce::jlimit(std::max(0, floorTick), end - 1,
                                       (int) std::llround(snapped * Pattern::kTicksPerBeat));
            if (t != ci.startTick) host().clips().resize(node_, c, end - t, true);
            break;
        }
        case Drag::TrimR: {
            const int t = std::max(start0_ + 1, (int) std::llround(snapped * Pattern::kTicksPerBeat));
            host().clips().resize(node_, c, t - start0_, false);
            break;
        }
        case Drag::StretchR: {
            const int t = std::max(start0_ + 1, (int) std::llround(snapped * Pattern::kTicksPerBeat));
            const auto ci = host().clips().list(node_)[(size_t) c];
            if (t - start0_ != ci.lengthTicks) host().clips().stretch(node_, c, t - start0_);
            break;
        }
        case Drag::StretchL: {
            const int end = start0_ + len0_;
            const int t = juce::jlimit(0, end - 1, (int) std::llround(snapped * Pattern::kTicksPerBeat));
            const auto ci = host().clips().list(node_)[(size_t) c];
            if (t != ci.startTick) {
                host().clips().stretch(node_, c, end - t);
                host().clips().move(node_, c, t);
            }
            break;
        }
        case Drag::Slip: {
            const auto delta = (long long) std::llround((anchorBeat_ - beat) * samplesPerBeat());
            const auto cur = host().clips().list(node_)[(size_t) c].audioOffset;
            host().clips().slip(node_, c, offset0_ + delta - cur);
            break;
        }
        case Drag::FadeL:
        case Drag::FadeR: {
            const auto ci = host().clips().list(node_)[(size_t) c];
            const int t = (int) std::llround(snapped * Pattern::kTicksPerBeat);
            if (drag_ == Drag::FadeL)
                host().clips().setFades(node_, c, juce::jlimit(0, ci.lengthTicks, t - ci.startTick), ci.fadeOutTicks);
            else
                host().clips().setFades(node_, c, ci.fadeInTicks,
                                       juce::jlimit(0, ci.lengthTicks, ci.startTick + ci.lengthTicks - t));
            break;
        }
        case Drag::CurveL:
        case Drag::CurveR: {
            const auto ci = host().clips().list(node_)[(size_t) c];
            const double v = timelinechrome::fadeCurveFromDrag(curve0_, pp.y - curveY0_,
                                                               clipBox().getHeight());
            if (drag_ == Drag::CurveL)
                host().clips().setFadeCurves(node_, c, v, ci.fadeOutCurve);
            else
                host().clips().setFadeCurves(node_, c, ci.fadeInCurve, v);
            break;
        }
        case Drag::None: break;
    }
    if (drag_ != Drag::Select && drag_ != Drag::Pending) repaint();
}

void ClipEditorView::mouseUpClip() {
    const bool selecting = drag_ == Drag::Pending || drag_ == Drag::Select;
    if (drag_ == Drag::Pending) {
        view_.sel.active = false;
        host().setPositionBeats(ctx_.snapBeats(anchorBeat_, false));
    }
    drag_ = Drag::None;
    if (!selecting) repaint();
}

bool ClipEditorView::keyPressedClip(const juce::KeyPress& k) {
    const auto cmd = juce::ModifierKeys::commandModifier;
    const auto shift = juce::ModifierKeys::shiftModifier;
    if (k.getKeyCode() == juce::KeyPress::escapeKey && view_.sel.active) { view_.sel.active = false; repaint(); return true; }
    if (k.getKeyCode() == juce::KeyPress::escapeKey
        || k.getKeyCode() == juce::KeyPress::returnKey) { ctx_.leaveClip(); return true; }
    if (k.getKeyCode() == juce::KeyPress::tabKey)
        return tabToTransient(k.getModifiers().testFlags(shift) ? -1 : 1);
    if (k == juce::KeyPress('s')) { splitClipSelection(); return true; }
    if (k == juce::KeyPress('s', shift, 0)) { splitAtTransients(); return true; }
    if (k == juce::KeyPress('z')) { zoomToSelection(); return true; }
    if (k == juce::KeyPress('r')) { toggleClipReverse(); return true; }
    if (k == juce::KeyPress('l')) { loopClipSelection(); return true; }
    if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey)
        return deleteClipSelection(k.getModifiers().testFlags(shift));
    if (k == juce::KeyPress('a', cmd, 0)) {
        ClipEditor::ClipInfo ci;
        if (clipInfo(ci)) {
            view_.sel.active = true;
            view_.sel.from = ci.startTick / (double) Pattern::kTicksPerBeat;
            view_.sel.to = (ci.startTick + ci.lengthTicks) / (double) Pattern::kTicksPerBeat;
        }
        repaint();
        return true;
    }
    if (k == juce::KeyPress('c', cmd, 0)) { copyClipSelection(); return true; }
    if (k == juce::KeyPress('x', cmd, 0)) { copyClipSelection(); deleteClipSelection(false); return true; }
    if (k == juce::KeyPress('v', cmd, 0))
        return pasteClipSelection((int) std::llround(
            ctx_.snapBeats(host().positionBeats(), false) * Pattern::kTicksPerBeat));
    if (k == juce::KeyPress('1')) { ctx_.selectTool(Tool::Pointer); return true; }
    if (k == juce::KeyPress('3')) { ctx_.selectTool(Tool::Scissors); return true; }
    return false;
}

}
