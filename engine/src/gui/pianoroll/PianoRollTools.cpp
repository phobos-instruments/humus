// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/PianoRollEditor.h"

#include "hum/Swing.h"

#include <climits>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

std::vector<NoteEvent> PianoRollEditor::sharedClipboard_;

void PianoRollEditor::setTool(Tool t) {
    tool_ = t;
    toolP_.setToggleState(t == Tool::Pointer, juce::dontSendNotification);
    toolD_.setToggleState(t == Tool::Draw, juce::dontSendNotification);
    toolS_.setToggleState(t == Tool::Scissors, juce::dontSendNotification);
    toolE_.setToggleState(t == Tool::Eraser, juce::dontSendNotification);
    setMouseCursor(t == Tool::Draw   ? noteedit::pencilCursor()
                 : t == Tool::Eraser ? juce::MouseCursor(juce::MouseCursor::CrosshairCursor)
                                     : juce::MouseCursor(juce::MouseCursor::NormalCursor));
    repaint();
}

void PianoRollEditor::selectAllNotes() {
    selection_.clear();
    const int n = (int) notes().size();
    for (int i = 0; i < n; ++i) selection_.insert(i);
    repaint();
}

void PianoRollEditor::applyMarquee(juce::Rectangle<int> area, bool additive) {
    if (!additive) selection_.clear();
    const auto n = notes();
    for (int i = 0; i < (int) n.size(); ++i) {
        const auto& e = n[(size_t) i];
        const juce::Rectangle<int> r((int) tickToX(e.tick), (int) pitchToY(e.pitch),
                                     juce::jmax(3, (int) (tickToX(e.tick + e.lengthTicks)
                                                          - tickToX(e.tick))),
                                     kRowH - 1);
        if (area.intersects(r)) selection_.insert(i);
    }
    repaint();
}

void PianoRollEditor::nudgeSelection(int dTicks, int dSemis, int dVel) {
    if (selection_.empty() || (dTicks == 0 && dSemis == 0 && dVel == 0)) return;
    if (!nudgeOpen_) { host_.pushUndo(); nudgeOpen_ = true; }
    auto n = notes();
    const int span = durationTicks();
    for (int i : selection_) {
        if (i < 0 || i >= (int) n.size()) continue;
        auto& e = n[(size_t) i];
        e.tick = juce::jlimit(0, juce::jmax(0, span - 1), e.tick + dTicks);
        e.pitch = juce::jlimit(0, kMidiMax, e.pitch + dSemis);
        e.velocity = juce::jlimit(1, kMidiMax, e.velocity + dVel);
    }
    commit(n);
}

void PianoRollEditor::printGrooveToSelection() {
    if (selection_.empty()) return;
    const bool follow = host_.liveParamValue(name_, params_.swingFollow) >= 0.5;
    const auto g = follow
                       ? swing::grooveFor(host_.groove(), host_.grooveUnit())
                       : swing::grooveFor(host_.liveParamValue(name_, params_.swing),
                                          host_.liveParamValue(name_, params_.swingUnit) < 0.5
                                              ? "1/8" : "1/16");
    if (g.amount <= 0.0) return;
    host_.pushUndo();
    auto n = notes();
    const int span = durationTicks();
    for (int i : selection_) {
        if (i < 0 || i >= (int) n.size()) continue;
        auto& e = n[(size_t) i];
        e.tick = juce::jlimit(0, juce::jmax(0, span - 1),
                              e.tick + (int) std::lround(swing::delayTicks(e.tick, g)));
    }
    commit(n);
}

void PianoRollEditor::deleteSelection() {
    if (selection_.empty()) return;
    host_.pushUndo();
    auto n = notes();
    std::vector<NoteEvent> kept;
    for (int i = 0; i < (int) n.size(); ++i)
        if (!selection_.count(i)) kept.push_back(n[(size_t) i]);
    selection_.clear();
    commit(kept);
}

void PianoRollEditor::copySelection(bool cut) {
    const auto n = notes();
    if (selection_.empty()) return;
    int minTick = INT_MAX;
    for (int i : selection_)
        if (i < (int) n.size()) minTick = std::min(minTick, n[(size_t) i].tick);
    sharedClipboard_.clear();
    for (int i : selection_)
        if (i < (int) n.size()) {
            auto e = n[(size_t) i];
            e.tick -= minTick;
            sharedClipboard_.push_back(e);
        }
    if (cut) deleteSelection();
}

void PianoRollEditor::pasteClipboard() {
    if (sharedClipboard_.empty()) return;
    host_.pushUndo();
    const double ph = playheadClipTick();
    const int at = snapTick(ph >= 0.0 ? (int) std::lround(ph) : 0);
    auto n = notes();
    selection_.clear();
    for (auto e : sharedClipboard_) {
        e.tick += at;
        if (e.tick >= durationTicks()) continue;
        selection_.insert((int) n.size());
        n.push_back(e);
    }
    commit(n);
}

void PianoRollEditor::duplicateSelection() {
    const auto base = notes();
    if (selection_.empty()) return;
    host_.pushUndo();
    int minTick = INT_MAX, maxEnd = 0;
    for (int i : selection_)
        if (i < (int) base.size()) {
            minTick = std::min(minTick, base[(size_t) i].tick);
            maxEnd = std::max(maxEnd, base[(size_t) i].tick + base[(size_t) i].lengthTicks);
        }
    const int s = snapTicks();
    const int span = ((maxEnd - minTick + s - 1) / s) * s;
    auto n = base;
    std::set<int> fresh;
    for (int i : selection_)
        if (i < (int) base.size()) {
            auto e = base[(size_t) i];
            e.tick += span;
            if (e.tick >= durationTicks()) continue;
            fresh.insert((int) n.size());
            n.push_back(e);
        }
    selection_ = std::move(fresh);
    commit(n);
}

void PianoRollEditor::splitNoteAt(int noteIndex, int atTick) {
    auto n = notes();
    if (noteIndex < 0 || noteIndex >= (int) n.size()) return;
    auto& e = n[(size_t) noteIndex];
    const int cut = snapTick(atTick);
    if (cut <= e.tick || cut >= e.tick + e.lengthTicks) return;
    host_.pushUndo();
    NoteEvent right = e;
    right.tick = cut;
    right.lengthTicks = e.tick + e.lengthTicks - cut;
    e.lengthTicks = cut - e.tick;
    n.push_back(right);
    selection_.clear();
    commit(n);
}

bool PianoRollEditor::keyPressed(const juce::KeyPress& k) {
    using juce::KeyPress;
    const auto cmd = juce::ModifierKeys::commandModifier;
    if (k.getModifiers().isAnyModifierKeyDown() == false) {
        switch (k.getTextCharacter()) {
            case '1': setTool(Tool::Pointer); return true;
            case '2': setTool(Tool::Draw); return true;
            case '3': setTool(Tool::Scissors); return true;
            case '4': setTool(Tool::Eraser); return true;
            case 'r': case 'R': loopTap(); return true;
            default: break;
        }
        if (k.getKeyCode() == KeyPress::deleteKey ||
            k.getKeyCode() == KeyPress::backspaceKey) {
            if (selection_.empty()) return false;
            deleteSelection();
            return true;
        }
        if (k.getKeyCode() == KeyPress::escapeKey) {
            if (selection_.empty()) return false;
            selection_.clear();
            repaint();
            return true;
        }
        if (k == KeyPress(KeyPress::upKey))    { nudgeSelection(0, 1, 0);  return true; }
        if (k == KeyPress(KeyPress::downKey))  { nudgeSelection(0, -1, 0); return true; }
        if (k == KeyPress(KeyPress::leftKey))  { nudgeSelection(-snapTicks(), 0, 0); return true; }
        if (k == KeyPress(KeyPress::rightKey)) { nudgeSelection(snapTicks(), 0, 0);  return true; }
        return false;
    }
    const auto shift = juce::ModifierKeys::shiftModifier;
    const int oct = juce::jmax(1, noteedit::octaveSteps(host_.model()));
    if (k == KeyPress(KeyPress::upKey, shift, 0))   { nudgeSelection(0, oct, 0);  return true; }
    if (k == KeyPress(KeyPress::downKey, shift, 0)) { nudgeSelection(0, -oct, 0); return true; }
    if (k == KeyPress(KeyPress::upKey, cmd, 0))     { nudgeSelection(0, 0, 10);  return true; }
    if (k == KeyPress(KeyPress::downKey, cmd, 0))   { nudgeSelection(0, 0, -10); return true; }
    if (k == KeyPress('a', cmd, 0)) { selectAllNotes(); return true; }
    if (k == KeyPress('c', cmd, 0)) { copySelection(false); return true; }
    if (k == KeyPress('x', cmd, 0)) { copySelection(true); return true; }
    if (k == KeyPress('v', cmd, 0)) { pasteClipboard(); return true; }
    if (k == KeyPress('d', cmd, 0)) { duplicateSelection(); return true; }
    return false;
}

}
