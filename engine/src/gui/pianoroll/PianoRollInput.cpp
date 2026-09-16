// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/PianoRollEditor.h"
#include "gui/host/EngineHostClips.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

void PianoRollEditor::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();
    nudgeOpen_ = false;
    const auto p = e.getPosition();
    if (p.y >= gridBottom() && p.y < gridBottom() + kVelH && p.x >= gridLeft()) {
        if (e.mods.isPopupMenu()) { showLaneMenu(); return; }
        host_.pushUndo();
        if (laneCC_ < 0) {
            gestureNotes_ = notes();
            gesture_ = Gesture::VelLane;
            applyVelocityLane(p);
        } else {
            gestureCCs_ = host_.clips().ccs(name_, clip_);
            gesture_ = Gesture::CCLane;
            applyCCLane(p);
        }
        return;
    }
    if (p.x < gridLeft() && p.y >= gridTop() && p.y < gridBottom()
        && !e.mods.isPopupMenu()) {
        gesture_ = Gesture::Keys;
        soundKey(pitchAt(p.y));
        return;
    }
    if (p.y < gridTop() || p.y >= gridBottom() || p.x < gridLeft()) return;
    const int pitch = pitchAt(p.y);
    if (pitch < 0 || pitch > kMidiMax) return;
    const int tick = juce::jlimit(0, durationTicks() - 1, xToTick((float) p.x));

    noteedit::Grab grab = noteedit::Grab::Miss;
    const int hit = noteAt(tick, pitch, grab, p.x);

    if (e.mods.isPopupMenu()) {
        if (!selection_.empty()) {
            juce::PopupMenu m;
            m.addItem(1, tr("piano-roll-input.print-groove-into-selection", "Print groove into selection"));
            m.addSeparator();
            m.addItem(2, tr("piano-roll-input.delete-selection", "Delete selection"));
            m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
                if (r == 1) printGrooveToSelection();
                else if (r == 2) deleteSelection();
            });
            return;
        }
        if (hit >= 0) {
            host_.pushUndo();
            auto n = notes();
            n.erase(n.begin() + hit);
            selection_.clear();
            commit(n);
        }
        return;
    }

    if (tool_ == Tool::Scissors) {
        if (hit >= 0) splitNoteAt(hit, tick);
        return;
    }

    if (tool_ == Tool::Eraser) {
        host_.pushUndo();
        gestureNotes_ = notes();
        selection_.clear();
        gesture_ = Gesture::Erase;
        if (hit >= 0) gestureNotes_.erase(gestureNotes_.begin() + hit);
        repaint();
        return;
    }

    dragStart_ = p;

    if (tool_ == Tool::Pointer) {
        if (hit < 0) {
            if (!e.mods.isShiftDown()) selection_.clear();
            gesture_ = Gesture::Marquee;
            marquee_ = {p.x, p.y, 0, 0};
            repaint();
            return;
        }
        if (e.mods.isShiftDown()) {
            if (selection_.count(hit)) selection_.erase(hit);
            else selection_.insert(hit);
            repaint();
            return;
        }
        if (!selection_.count(hit)) { selection_.clear(); selection_.insert(hit); }
        if (selection_.size() > 1 && grab == noteedit::Grab::Body && !e.mods.isAltDown()) {
            host_.pushUndo();
            gestureBase_ = notes();
            gestureNotes_ = gestureBase_;
            gesture_ = Gesture::MoveGroup;
            gestureStartTick_ = tick;
            gestureStartPitch_ = pitch;
            gestureIndex_ = -1;
            repaint();
            return;
        }
    }

    host_.pushUndo();
    gestureNotes_ = notes();

    if (hit >= 0 && e.mods.isAltDown()) {
        gesture_ = Gesture::Velocity;
        gestureIndex_ = hit;
        gestureStartVel_ = gestureNotes_[(size_t) hit].velocity;
    } else if (hit >= 0 && grab == noteedit::Grab::RightEdge) {
        gesture_ = Gesture::Resize;
        gestureIndex_ = hit;
    } else if (hit >= 0 && grab == noteedit::Grab::LeftEdge) {
        gesture_ = Gesture::ResizeL;
        gestureIndex_ = hit;
    } else if (hit >= 0) {
        gesture_ = Gesture::Move;
        gestureIndex_ = hit;
        selection_.clear(); selection_.insert(hit);
        gestureStartTick_ = gestureNotes_[(size_t) hit].tick;
        gestureStartPitch_ = gestureNotes_[(size_t) hit].pitch;
        gestureTickOffset_ = tick - gestureNotes_[(size_t) hit].tick;
    } else if (tool_ == Tool::Draw) {
        gesture_ = Gesture::Create;
        NoteEvent n;
        n.tick = snapTick(tick);
        n.pitch = pitch;
        n.lengthTicks = snapTicks();
        n.velocity = 100;
        selection_.clear();
        gestureNotes_.push_back(n);
        gestureIndex_ = (int) gestureNotes_.size() - 1;
    } else {
        gesture_ = Gesture::None;
        return;
    }
    repaint();
}

void PianoRollEditor::mouseDrag(const juce::MouseEvent& e) {
    if (gesture_ == Gesture::Keys) { soundKey(pitchAt(e.getPosition().y)); return; }
    const auto p = e.getPosition();
    const int tick = xToTick((float) p.x);

    if (gesture_ == Gesture::VelLane) {
        applyVelocityLane(p);
        return;
    }
    if (gesture_ == Gesture::CCLane) {
        applyCCLane(p);
        return;
    }
    if (gesture_ == Gesture::Marquee) {
        marquee_ = juce::Rectangle<int>(dragStart_, p);
        applyMarquee(marquee_, e.mods.isShiftDown());
        return;
    }
    if (gesture_ == Gesture::Erase) {
        bool nearEdge = false;
        const int pitch = pitchAt(p.y);
        for (int i = (int) gestureNotes_.size() - 1; i >= 0; --i) {
            const auto& n = gestureNotes_[(size_t) i];
            if (n.pitch == pitch && tick >= n.tick && tick < n.tick + n.lengthTicks) {
                gestureNotes_.erase(gestureNotes_.begin() + i);
                repaint();
                break;
            }
        }
        juce::ignoreUnused(nearEdge);
        return;
    }
    if (gesture_ == Gesture::MoveGroup) {
        const int dTick = noteedit::snapDelta(tick - gestureStartTick_, snapTicks());
        const int dPitch = juce::jlimit(0, kMidiMax, pitchAt(p.y)) - gestureStartPitch_;
        gestureNotes_ = gestureBase_;
        for (int i : selection_) {
            if (i < 0 || i >= (int) gestureNotes_.size()) continue;
            auto& n = gestureNotes_[(size_t) i];
            n.tick = juce::jlimit(0, durationTicks() - 1, n.tick + dTick);
            n.pitch = juce::jlimit(0, kMidiMax, n.pitch + dPitch);
        }
        repaint();
        return;
    }

    if (gesture_ == Gesture::None || gestureIndex_ < 0
        || gestureIndex_ >= (int) gestureNotes_.size()) return;
    auto& n = gestureNotes_[(size_t) gestureIndex_];

    switch (gesture_) {
        case Gesture::Create:
        case Gesture::Resize: {
            const int end = juce::jlimit(n.tick + 1, durationTicks(), tick);
            const int snapped = snapTick(end - 1) + snapTicks();
            n.lengthTicks = juce::jmax(snapTicks() / 2, snapped - n.tick);
            break;
        }
        case Gesture::ResizeL: {
            const int head = snapTick(juce::jlimit(0, durationTicks() - 1, tick));
            const auto r = noteedit::resizeLeft(n.tick, n.lengthTicks, head,
                                                juce::jmax(1, snapTicks() / 2));
            n.tick = r.tick;
            n.lengthTicks = r.lengthTicks;
            break;
        }
        case Gesture::Move: {
            const int moved = noteedit::snapDelta(
                (tick - gestureTickOffset_) - gestureStartTick_, snapTicks());
            n.tick = juce::jlimit(0, durationTicks() - 1, gestureStartTick_ + moved);
            n.pitch = juce::jlimit(0, kMidiMax, pitchAt(p.y));
            break;
        }
        case Gesture::Velocity: {
            n.velocity = juce::jlimit(1, kMidiMax, gestureStartVel_ + (dragStart_.y - p.y));
            break;
        }
        default: break;
    }
    repaint();
}

void PianoRollEditor::mouseUp(const juce::MouseEvent&) {
    if (gesture_ == Gesture::Keys) { releaseKey(); gesture_ = Gesture::None; return; }
    if (gesture_ == Gesture::None) return;
    if (gesture_ == Gesture::Marquee) {
        gesture_ = Gesture::None;
        marquee_ = {};
        repaint();
        return;
    }
    if (gesture_ == Gesture::CCLane) {
        host_.clips().setCCs(name_, clip_, gestureCCs_);
        gesture_ = Gesture::None;
        gestureCCs_.clear();
        repaint();
        return;
    }
    commit(gestureNotes_);
    gesture_ = Gesture::None;
    gestureIndex_ = -1;
    gestureNotes_.clear();
    gestureBase_.clear();
}

void PianoRollEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    if (p.y < gridTop() || p.y >= gridBottom() || p.x < gridLeft()) return;
    noteedit::Grab grab = noteedit::Grab::Miss;
    const int pitch = pitchAt(p.y);
    const int tick = juce::jlimit(0, durationTicks() - 1, xToTick((float) p.x));
    const int hit = noteAt(tick, pitch, grab, p.x);
    if (hit >= 0) {
        host_.pushUndo();
        auto n = notes();
        n.erase(n.begin() + hit);
        selection_.clear();
        commit(n);
    } else if (tool_ == Tool::Pointer) {
        host_.pushUndo();
        auto n = notes();
        NoteEvent ne;
        ne.tick = snapTick(tick);
        ne.pitch = pitch;
        ne.lengthTicks = snapTicks();
        ne.velocity = 100;
        selection_.clear();
        selection_.insert((int) n.size());
        n.push_back(ne);
        commit(n);
    }
}

void PianoRollEditor::mouseMove(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    if (p.y < gridTop() || p.x < gridLeft()) { setMouseCursor(juce::MouseCursor::NormalCursor); return; }
    if (tool_ == Tool::Draw) { setMouseCursor(noteedit::pencilCursor()); return; }
    if (tool_ == Tool::Eraser) { setMouseCursor(juce::MouseCursor::CrosshairCursor); return; }
    if (tool_ == Tool::Scissors) { setMouseCursor(juce::MouseCursor::IBeamCursor); return; }
    noteedit::Grab grab = noteedit::Grab::Miss;
    const int hit = noteAt(xToTick((float) p.x), pitchAt(p.y), grab, p.x);
    setMouseCursor(hit < 0                    ? juce::MouseCursor::NormalCursor
                 : grab == noteedit::Grab::Body ? juce::MouseCursor::DraggingHandCursor
                                               : noteedit::cursorFor(grab));
}

void PianoRollEditor::mouseWheelMove(const juce::MouseEvent& e,
                                     const juce::MouseWheelDetails& wheel) {
    if (e.getPosition().y < gridTop()) return;
    const int step = pitchWheel_.add(wheel.deltaY, 12.0f);
    if (step == 0) return;
    const int rows = (gridBottom() - gridTop()) / kRowH;
    topPitch_ = juce::jlimit(juce::jmax(0, rows - 1), kMidiMax, topPitch_ + step);
    pitchScrolled_ = true;
    repaint();
}

}
