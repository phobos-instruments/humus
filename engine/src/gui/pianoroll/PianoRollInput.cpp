// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/PianoRollEditor.h"
#include "gui/host/EngineHostAutomation.h"
#include "gui/host/EngineHostClips.h"

#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
roll::Mods modsOf(const juce::MouseEvent& e) {
    roll::Mods m;
    m.shift = e.mods.isShiftDown();
    m.alt = e.mods.isAltDown();
    m.command = e.mods.isCommandDown();
    m.ctrl = e.mods.isCtrlDown();
    m.popup = e.mods.isPopupMenu();
    return m;
}
}

roll::Geometry PianoRollEditor::geometry() const {
    roll::Geometry g;
    g.gridLeft = gridLeft();
    g.gridTop = gridTop();
    g.gridBottom = gridBottom();
    g.rowH = kRowH;
    g.velH = kVelH;
    g.topPitch = model_.topPitch;
    g.duration = durationTicks();
    g.ppt = (double) juce::jmax(120, getWidth() - kKeyW) / (double) juce::jmax(1, g.duration);
    g.snap = snapTicks();
    g.free = snapFree();
    g.barTicks = juce::jmax(1, (int) std::lround(host_.automation().timeSig().quarterNotesPerBar() * Pattern::kTicksPerBeat));
    return g;
}

std::vector<NoteEvent> PianoRollEditor::Clip::notes() const {
    return editor_.host_.clips().notes(editor_.name_, editor_.clip_);
}

void PianoRollEditor::Clip::setNotes(const std::vector<NoteEvent>& n, int durationTicks) {
    editor_.host_.clips().setNotes(editor_.name_, editor_.clip_, n, durationTicks);
    editor_.repaint();
}

std::vector<CCEvent> PianoRollEditor::Clip::ccs() const {
    return editor_.host_.clips().ccs(editor_.name_, editor_.clip_);
}

void PianoRollEditor::Clip::setCCs(const std::vector<CCEvent>& ccs) {
    editor_.host_.clips().setCCs(editor_.name_, editor_.clip_, ccs);
}

void PianoRollEditor::Clip::pushUndo() { editor_.host_.pushUndo(); }

void PianoRollEditor::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();
    const auto p = e.getPosition();
    int hit = -1;
    using Press = roll::RollModel::Press;
    switch (model_.press(geometry(), p.x, p.y, modsOf(e), hit)) {
        case Press::NoteMenu: showNoteMenu(hit); return;
        case Press::LaneMenu: showLaneMenu(); return;
        case Press::Keys: soundKey(model_.keyPitch()); return;
        case Press::Changed: repaint(); return;
        default: return;
    }
}

void PianoRollEditor::mouseDrag(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    const bool keys = model_.gesture() == Gesture::Keys;
    if (!model_.drag(geometry(), p.x, p.y, modsOf(e))) return;
    if (keys) soundKey(model_.keyPitch());
    else repaint();
}

void PianoRollEditor::mouseUp(const juce::MouseEvent&) {
    const bool keys = model_.gesture() == Gesture::Keys;
    const bool changed = model_.release(geometry());
    if (keys) releaseKey();
    if (changed) repaint();
}

void PianoRollEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    if (model_.doubleClick(geometry(), p.x, p.y)) repaint();
}

bool PianoRollEditor::cutGuideAt(juce::Point<int> p) const {
    return p.x >= gridLeft() && p.y >= gridTop() && p.y < gridBottom();
}

float PianoRollEditor::cutGuideX(int hoverX) const {
    const auto geo = geometry();
    return geo.tickToX(geo.snapTick(geo.clampTick(geo.xToTick((float) hoverX))));
}

void PianoRollEditor::trackHover(juce::Point<int> p) {
    const auto was = hover_;
    hover_ = p;
    if (model_.tool != Tool::Scissors) return;
    for (const auto pt : {was, hover_})
        if (cutGuideAt(pt))
            repaint((int) cutGuideX(pt.x) - 3, gridTop(), 7, gridBottom() - gridTop());
}

void PianoRollEditor::mouseExit(const juce::MouseEvent&) { trackHover({-1, -1}); }

void PianoRollEditor::mouseMove(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    trackHover(p);
    if (p.y < gridTop() || p.x < gridLeft()) { setMouseCursor(juce::MouseCursor::NormalCursor); return; }
    if (model_.tool != Tool::Pointer) {
        setMouseCursor(timelinechrome::toolCursor(model_.tool));
        return;
    }
    noteedit::Grab grab = noteedit::Grab::Miss;
    const auto geo = geometry();
    const int hit = model_.noteAt(geo, geo.xToTick((float) p.x), geo.pitchAt(p.y), grab, p.x);
    setMouseCursor(hit < 0                    ? juce::MouseCursor::NormalCursor
                 : grab == noteedit::Grab::Body ? juce::MouseCursor::DraggingHandCursor
                                               : noteedit::cursorFor(grab));
}

void PianoRollEditor::mouseWheelMove(const juce::MouseEvent& e,
                                     const juce::MouseWheelDetails& wheel) {
    if (e.getPosition().y < gridTop()) return;
    const int step = pitchWheel_.add(wheel.deltaY, 12.0f);
    if (step == 0) return;
    model_.scrollPitch(step, (gridBottom() - gridTop()) / kRowH);
    repaint();
}

}
