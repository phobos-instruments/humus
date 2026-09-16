// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TrackRollView.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <iterator>

#include "core/timeline/ClipOps.h"
#include "gui/common/Localisation.h"
#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/tracks/CutGuide.h"
#include "io/PatchDocument.h"

#include "hum/dsp/DspMath.h"
#include "gui/tracks/NotePlot.h"
#include "gui/tracks/TimelineTools.h"

namespace hum {

namespace {
using Tool = noteedit::Tool;
}

void TrackRollView::open(const std::string& node) {
    node_ = node;
    drag_ = Drag::None;
    const int under = clipOwning((int) std::llround(host().positionBeats() * Pattern::kTicksPerBeat));
    selClip_ = under >= 0 ? under : host().clips().list(node_).empty() ? -1 : 0;
    setVisible(true);
    fitRoll();
}

void TrackRollView::close() {
    node_.clear();
    sel_.clear();
    selClip_ = -1;
    drag_ = Drag::None;
    setVisible(false);
}

void TrackRollView::selectClip(int clip) {
    selClip_ = clip < (int) host().clips().list(node_).size() ? clip : -1;
    repaint();
}

juce::Rectangle<int> TrackRollView::clipHandle(bool left) const {
    const auto clips = host().clips().list(node_);
    if (selClip_ < 0 || selClip_ >= (int) clips.size()) return {};
    const auto& ci = clips[(size_t) selClip_];
    const float x = tickToX(left ? ci.startTick : ci.startTick + ci.lengthTicks);
    return {(int) std::lround(x) - 5, ribbonTop(), 10, kRibbonH + 10};
}

bool TrackRollView::clipHandleAt(juce::Point<int> p, bool& left) const {
    if (selClip_ < 0) return false;
    for (const bool side : {true, false})
        if (clipHandle(side).contains(p)) { left = side; return true; }
    return false;
}

void TrackRollView::resizeSelectedClipTo(int absTick, bool fromLeft) {
    const auto clips = host().clips().list(node_);
    if (selClip_ < 0 || selClip_ >= (int) clips.size()) return;
    const auto& ci = clips[(size_t) selClip_];
    const int grid = std::max(1, (int) std::llround(ctx_.gridBeats() * Pattern::kTicksPerBeat));
    const int len = fromLeft ? ci.startTick + ci.lengthTicks - absTick : absTick - ci.startTick;
    const int newLen = std::max(grid, len);
    if (newLen == ci.lengthTicks) return;
    host().clips().resize(node_, selClip_, newLen, fromLeft);
    repaint();
}

void TrackRollView::setRowHeight(float h) {
    rowH_ = h;
    repaint();
}

void TrackRollView::clearNoteSelection() {
    sel_.clear();
    repaint();
}

int TrackRollView::velAtY(int y) const {
    const int laneTop = getBottom() - velH_;
    const double f = 1.0 - (double) (y - laneTop - 3) / (double) (velH_ - 6);
    return juce::jlimit(1, kMidiMax, (int) std::lround(f * kMidiMaxD));
}

void TrackRollView::repaintRollKeys() {
    repaint(tracksgeo::kStripW - kKeyW, 0, kKeyW, getHeight());
}

void TrackRollView::resized() {
    if (!node_.empty() && scrollSemis_ == 0 && scrollAcc_ == 0.0f) fitRoll();
}

void TrackRollView::mouseDown(const juce::MouseEvent& e) { mouseDownRoll(e, toPane(e.getPosition())); }

void TrackRollView::mouseDrag(const juce::MouseEvent& e) {
    ctx_.edgeScroll(toPane(e.getPosition()));
    if (keyNote_ >= 0) {
        if (const auto rp = rollPlot(); rp.usable) soundRollKey(rp.pitchAt((float) toPane(e.getPosition()).y));
        return;
    }
    if (drag_ != Drag::None) mouseDragRoll(e);
}

void TrackRollView::mouseUp(const juce::MouseEvent&) { mouseUpRoll(); }

void TrackRollView::repaintCutGuide() { cutguide::repaintMove(*this, view_, ctx_, hover_.x, hover_.x); }

void TrackRollView::mouseMove(const juce::MouseEvent& e) {
    const auto p = toPane(e.getPosition());
    const auto was = hover_;
    hover_ = p;
    const Tool tool = ctx_.effectiveTool();
    if (tool == Tool::Scissors) cutguide::repaintMove(*this, view_, ctx_, was.x, p.x);
    if (bool left = false; tool == Tool::Pointer && clipHandleAt(p, left)) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }
    if (rollPlot().usable && rollShowsVelocity() && p.x >= tracksgeo::kStripW
        && p.y >= getBottom() - velH_ - 3 && p.y < getBottom()) {
        if (std::abs(p.y - (getBottom() - velH_)) <= 3) {
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            return;
        }
        if (tool != Tool::Pointer) { setMouseCursor(timelinechrome::toolCursor(tool)); return; }
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        return;
    }
    if (rollPlot().usable && rollField().contains(p)) {
        if (tool != Tool::Pointer) { setMouseCursor(timelinechrome::toolCursor(tool)); return; }
        int clip = -1, index = -1;
        bool nl = false, nr = false;
        noteAt(p, clip, index, nl, nr);
        setMouseCursor(index < 0 ? juce::MouseCursor(juce::MouseCursor::NormalCursor)
                     : nl || nr  ? juce::MouseCursor(juce::MouseCursor::LeftRightResizeCursor)
                                 : juce::MouseCursor(juce::MouseCursor::DraggingHandCursor));
        return;
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void TrackRollView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (e.mods.isCommandDown() || e.mods.isCtrlDown() || e.mods.isShiftDown()) {
        juce::Component::mouseWheelMove(e, wheel);
        return;
    }
    if (wheel.deltaX != 0.0f) {
        view_.scrollBeats = std::max(0.0, view_.scrollBeats
                                         - wheel.deltaX * 0.5 * (getWidth() - tracksgeo::kStripW) / view_.ppb);
        ctx_.viewChanged();
    }
    if (wheel.deltaY == 0.0f) return;
    if (e.mods.isAltDown()) {
        rowH_ = juce::jlimit(3.0f, 24.0f, rowH_ * (wheel.deltaY > 0 ? 1.15f : 1.0f / 1.15f));
        ctx_.viewChanged();
        return;
    }
    scrollAcc_ += wheel.deltaY * 8.0f;
    const float whole = std::floor(scrollAcc_);
    if (whole == 0.0f) return;
    scrollAcc_ -= whole;
    const int lo = -topPitch_, hi = kMidiMax - topPitch_;
    const int want = scrollSemis_ + (int) whole;
    scrollSemis_ = juce::jlimit(juce::jmin(lo, 0), juce::jmax(hi, 0), want);
    if (scrollSemis_ != want) scrollAcc_ = 0.0f;
    repaint();
}

juce::String TrackRollView::getTooltip() {
    const auto p = hover_;
    if (!rollPlot().usable || !rollField().contains(p)) return {};
    const auto tool = ctx_.effectiveTool();
    if (tool == Tool::Draw) return tr("tracks-pane-roll.draw-a-note-drag-to", "Draw a note - drag to set its length");
    if (tool == Tool::Scissors) return tr("tracks-pane-roll.split-the-note-at-the", "Split the note at the click");
    if (tool == Tool::Eraser) return tr("tracks-pane-roll.sweep-to-delete-notes", "Sweep to delete notes");
    int clip = -1, index = -1;
    bool nl = false, nr = false;
    noteAt(p, clip, index, nl, nr);
    if (index < 0) return tr("tracks-pane-roll.drag-to-marquee-right-click", "Drag to marquee - right-click for the note menu");
    if (nl || nr) return tr("tracks-pane-roll.drag-the-edge-to-resize", "Drag the edge to resize - Alt-drag the body for velocity");
    return tr("tracks-pane-roll.drag-to-move-alt-drag", "Drag to move - Alt-drag for velocity, right-click for the menu");
}

void TrackRollView::fitRoll() {
    const auto f = rollField();
    int lo = 0, hi = 0;
    rollPitchRange(lo, hi);
    const auto fit = timelinechrome::rollPlot((float) f.getY(), (float) f.getHeight(),
                                              lo, hi, 0, 10.0f);
    topPitch_ = fit.topPitch;
    rowH_ = fit.rowH;
    scrollSemis_ = 0;
    scrollAcc_ = 0.0f;
}

bool TrackRollView::mouseDownRoll(const juce::MouseEvent& e, juce::Point<int> p) {
    if (node_.empty()) return false;
    if (mouseDownRollGrid(e, p)) return true;
    const auto field = rollField();
    if (p.x >= tracksgeo::kStripW - kKeyW && p.x < tracksgeo::kStripW
        && p.y >= field.getY() && p.y < field.getBottom()) {
        const auto rp = rollPlot();
        if (rp.usable) { soundRollKey(rp.pitchAt((float) p.y)); return true; }
    }
    return p.y >= tracksgeo::headerH();
}

juce::Rectangle<int> TrackRollView::rollField() const {
    const int top = tracksgeo::headerH() + tracksgeo::kChipH + kRibbonH;
    const int bottom = getBottom() - (rollShowsVelocity() ? velH_ : 0);
    return {tracksgeo::kStripW, top, std::max(0, getWidth() - tracksgeo::kStripW), std::max(0, bottom - top)};
}

timelinechrome::RollPlot TrackRollView::rollPlot() const {
    const auto f = rollField();
    timelinechrome::RollPlot rp;
    rp.rowH = rowH_;
    rp.top = (float) f.getY() + scrollAcc_ * rp.rowH;
    rp.h = (float) f.getHeight();
    rp.topPitch = juce::jlimit(0, kMidiMax, topPitch_ + scrollSemis_);
    rp.usable = f.getHeight() >= 12;
    return rp;
}

void TrackRollView::rollPitchRange(int& lo, int& hi) const {
    lo = kMidiMax; hi = 0;
    bool any = false;
    for (int c = 0; c < (int) host().clips().list(node_).size(); ++c)
        for (const auto& n : host().clips().notes(node_, c)) {
            lo = std::min(lo, n.pitch);
            hi = std::max(hi, n.pitch);
            any = true;
        }
    if (!any) { lo = 36; hi = 72; }
}

void TrackRollView::soundRollKey(int pitch) {
    pitch = juce::jlimit(0, kMidiMax, pitch);
    if (pitch == keyNote_) return;
    if (keyNote_ >= 0)
        host().injectLiveMidi(juce::MidiMessage::noteOff(1, keyNote_));
    keyNote_ = pitch;
    host().injectLiveMidi(juce::MidiMessage::noteOn(1, pitch, (juce::uint8) 100));
    repaintRollKeys();
}

int TrackRollView::clipOwning(int absTick) const {
    const auto clips = host().clips().list(node_);
    for (int c = (int) clips.size() - 1; c >= 0; --c)
        if (absTick >= clips[(size_t) c].startTick
            && absTick < clips[(size_t) c].startTick + clips[(size_t) c].lengthTicks)
            return c;
    return -1;
}

int TrackRollView::noteAt(juce::Point<int> p, int& clip, int& index,
                       bool& leftEdge, bool& rightEdge) const {
    clip = index = -1;
    leftEdge = rightEdge = false;
    const auto clips = host().clips().list(node_);
    for (int c = (int) clips.size() - 1; c >= 0; --c) {
        const auto notes = host().clips().notes(node_, c);
        for (int i = (int) notes.size() - 1; i >= 0; --i) {
            const auto b = noteBounds(clips[(size_t) c].startTick, notes[(size_t) i]);
            if (!b.expanded(0.0f, 1.0f).contains(p.toFloat())) continue;
            clip = c;
            index = i;
            const auto g = noteedit::grabAt(b, p.toFloat());
            leftEdge = g == noteedit::Grab::LeftEdge;
            rightEdge = g == noteedit::Grab::RightEdge;
            return i;
        }
    }
    return -1;
}

juce::Rectangle<float> TrackRollView::noteBounds(int clipStart, const NoteEvent& n) const {
    const auto rp = rollPlot();
    const float x = tickToX(clipStart + n.tick);
    const float w = std::max(2.0f, tickToX(clipStart + n.tick
                                           + std::max(1, n.lengthTicks)) - x);
    return {x, rp.yFor(n.pitch), w, std::max(2.0f, rp.hFor(n.pitch) - 1.0f)};
}

bool TrackRollView::mouseDownRollGrid(const juce::MouseEvent& e, juce::Point<int> p) {
    const auto rp = rollPlot();
    if (rp.usable && rollShowsVelocity() && !e.mods.isPopupMenu() && p.x >= tracksgeo::kStripW
        && std::abs(p.y - (getBottom() - velH_)) <= 3) {
        drag_ = Drag::VelDivider;
        return true;
    }
    if (rp.usable && rollShowsVelocity() && !e.mods.isPopupMenu() && p.x >= tracksgeo::kStripW
        && p.y >= getBottom() - velH_ && p.y < getBottom()) {
        host().pushUndo();
        drag_ = Drag::VelLane;
        velLine_ = ctx_.effectiveTool() == Tool::Line || e.mods.isShiftDown();
        velAnchor_ = p;
        if (velLine_) applyVelLaneLine(p, p);
        else applyVelLaneEdit(p);
        return true;
    }
    if (rp.usable && !e.mods.isPopupMenu() && ctx_.effectiveTool() == Tool::Pointer) {
        if (bool left = false; clipHandleAt(p, left)) {
            host().pushUndo();
            drag_ = left ? Drag::ClipL : Drag::ClipR;
            return true;
        }
        if (p.x >= tracksgeo::kStripW && p.y >= ribbonTop() && p.y < ribbonTop() + kRibbonH) {
            selectClip(clipOwning(std::max(0, xToTick((float) p.x))));
            return true;
        }
    }
    if (!rp.usable || !rollField().contains(p)) return false;
    nudgeRunUndoOpen_ = false;

    int clip = -1, index = -1;
    bool leftEdge = false, rightEdge = false;
    noteAt(p, clip, index, leftEdge, rightEdge);
    const int tick = std::max(0, xToTick((float) p.x));
    if (const int owner = index >= 0 ? clip : clipOwning(tick); owner >= 0 && !e.mods.isPopupMenu())
        selClip_ = owner;
    const int pitch = juce::jlimit(0, kMidiMax, rp.pitchAt((float) p.y));
    anchor_ = p;
    anchorTick_ = tick;
    anchorPitch_ = pitch;
    anchorVel_ = 0;

    if (e.mods.isPopupMenu()) {
        if (index >= 0 && sel_.count({clip, index}) == 0) {
            sel_.clear();
            sel_.insert({clip, index});
            repaint();
        }
        showNoteMenu(localPointToGlobal(p - getPosition()));
        return true;
    }

    const auto tool = ctx_.effectiveTool();
    if (tool == Tool::Eraser) {
        host().pushUndo();
        drag_ = Drag::Erase;
        eraseNoteUnder(p);
        return true;
    }
    if (tool == Tool::Scissors) {
        if (index < 0) return true;
        host().pushUndo();
        auto notes = host().clips().notes(node_, clip);
        int start = 0;
        for (const auto& ci : host().clips().list(node_))
            if (ci.index == clip) start = ci.startTick;
        const int cut = (int) std::llround(ctx_.snapBeats(xToBeat((float) p.x), e.mods.isAltDown())
                                           * Pattern::kTicksPerBeat) - start;
        auto& n = notes[(size_t) index];
        if (cut > n.tick + noteedit::kMinTicks && cut < n.tick + n.lengthTicks - noteedit::kMinTicks) {
            NoteEvent tail = n;
            tail.tick = cut;
            tail.lengthTicks = n.tick + n.lengthTicks - cut;
            n.lengthTicks = cut - n.tick;
            notes.push_back(tail);
            host().clips().setNotes(node_, clip, notes, 0);
        }
        sel_.clear();
        repaint();
        return true;
    }
    if (tool == Tool::Draw) {
        const int owner = clipOwning(tick);
        if (owner < 0) return true;
        host().pushUndo();
        int start = 0;
        for (const auto& ci : host().clips().list(node_))
            if (ci.index == owner) start = ci.startTick;
        const int at = (int) std::llround(ctx_.snapBeats(xToBeat((float) p.x), e.mods.isAltDown())
                                          * Pattern::kTicksPerBeat);
        auto notes = host().clips().notes(node_, owner);
        NoteEvent n;
        n.tick = std::max(0, at - start);
        n.pitch = pitch;
        n.lengthTicks = std::max(noteedit::kMinTicks,
                                 (int) std::llround(ctx_.gridBeats() * Pattern::kTicksPerBeat));
        n.velocity = 100;
        notes.push_back(n);
        host().clips().setNotes(node_, owner, notes, 0);
        sel_.clear();
        for (int i = 0; i < (int) notes.size(); ++i)
            if (notes[(size_t) i].tick == n.tick && notes[(size_t) i].pitch == n.pitch)
                sel_.insert({owner, i});
        anchorTick_ = at;
        snapshotNotes();
        drag_ = Drag::ResizeR;
        repaint();
        return true;
    }

    if (index < 0) {
        if (!e.mods.isShiftDown()) sel_.clear();
        marquee_ = juce::Rectangle<int>(p, p);
        drag_ = Drag::Marquee;
        repaint();
        return true;
    }
    const std::pair<int, int> hit{clip, index};
    if (e.mods.isShiftDown()) {
        if (!sel_.insert(hit).second) sel_.erase(hit);
        repaint();
        return true;
    }
    if (sel_.count(hit) == 0) { sel_.clear(); sel_.insert(hit); }
    host().pushUndo();
    snapshotNotes();
    if (e.mods.isAltDown()) {
        anchorVel_ = 1;
        drag_ = Drag::Velocity;
    } else {
        drag_ = leftEdge ? Drag::ResizeL
                  : rightEdge ? Drag::ResizeR : Drag::Move;
    }
    repaint();
    return true;
}

void TrackRollView::mouseDragRoll(const juce::MouseEvent& e) {
    const auto pe = toPane(e.getPosition());
    switch (drag_) {
        case Drag::None: return;
        case Drag::Erase: eraseNoteUnder(pe); return;
        case Drag::ClipL:
        case Drag::ClipR:
            resizeSelectedClipTo((int) std::llround(ctx_.snapBeats(xToBeat((float) pe.x), e.mods.isAltDown())
                                                    * Pattern::kTicksPerBeat),
                                 drag_ == Drag::ClipL);
            return;
        case Drag::VelDivider:
            velH_ = juce::jlimit(28, 160, getBottom() - pe.y);
            repaint();
            return;
        case Drag::VelLane:
            if (velLine_) applyVelLaneLine(velAnchor_, pe);
            else applyVelLaneEdit(pe);
            return;
        case Drag::Marquee: {
            marquee_ = juce::Rectangle<int>(anchor_, pe);
            sel_.clear();
            for (const auto& ci : host().clips().list(node_)) {
                const auto notes = host().clips().notes(node_, ci.index);
                for (int i = 0; i < (int) notes.size(); ++i)
                    if (marquee_.toFloat().intersects(
                            noteBounds(ci.startTick, notes[(size_t) i])))
                        sel_.insert({ci.index, i});
            }
            repaint();
            return;
        }
        default: applyNoteDrag(e); return;
    }
}

void TrackRollView::mouseUpRoll() {
    const bool clipEdge = drag_ == Drag::ClipL || drag_ == Drag::ClipR;
    if (keyNote_ >= 0) {
        host().injectLiveMidi(juce::MidiMessage::noteOff(1, keyNote_));
        keyNote_ = -1;
        repaintRollKeys();
    }
    drag_ = Drag::None;
    velLine_ = false;
    velShowX_ = -1;
    marquee_ = {};
    base_.clear();
    selBase_.clear();
    anchorVel_ = 0;
    repaint();
    if (clipEdge) ctx_.rebuildRows();
}

}
