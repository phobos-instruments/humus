// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/PianoRollEditor.h"

#include "hum/Swing.h"

#include <climits>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void PianoRollEditor::setTool(Tool t) {
    model_.tool = t;
    toolP_.setToggleState(t == Tool::Pointer, juce::dontSendNotification);
    toolD_.setToggleState(t == Tool::Draw, juce::dontSendNotification);
    toolS_.setToggleState(t == Tool::Scissors, juce::dontSendNotification);
    toolE_.setToggleState(t == Tool::Eraser, juce::dontSendNotification);
    setMouseCursor(timelinechrome::toolCursor(t));
    repaint();
}

void PianoRollEditor::selectAllNotes() {
    model_.selectAll();
    repaint();
}

void PianoRollEditor::nudgeSelection(int dTicks, int dSemis, int dVel) {
    model_.nudge(geometry(), dTicks, dSemis, dVel);
}

void PianoRollEditor::printGrooveToSelection() {
    if (model_.selection.empty()) return;
    const bool follow = host_.liveParamValue(name_, params_.swingFollow) >= 0.5;
    const auto g = follow
                       ? swing::grooveFor(host_.groove(), host_.grooveUnit())
                       : swing::grooveFor(host_.liveParamValue(name_, params_.swing),
                                          host_.liveParamValue(name_, params_.swingUnit) < 0.5
                                              ? "1/8" : "1/16");
    model_.printGroove(geometry(), g);
}

void PianoRollEditor::quantiseSelection(int gridTicks) {
    model_.quantise(geometry(), gridTicks);
    repaint();
}

void PianoRollEditor::deleteSelection() { model_.deleteSelection(geometry()); }

void PianoRollEditor::copySelection(bool cut) { model_.copySelection(geometry(), cut); }

void PianoRollEditor::pasteClipboard() { model_.paste(geometry(), playheadClipTick()); }

void PianoRollEditor::duplicateSelection() { model_.duplicate(geometry()); }

void PianoRollEditor::splitNoteAt(int noteIndex, int atTick) { model_.split(geometry(), noteIndex, atTick); }

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
            if (model_.selection.empty() && model_.ccSelection.empty()) return false;
            if (model_.selection.empty()) deleteSelectedCCs();
            else deleteSelection();
            return true;
        }
        if (k.getKeyCode() == KeyPress::escapeKey) {
            if (model_.selection.empty() && model_.ccSelection.empty()) return false;
            model_.selection.clear();
            model_.ccSelection.clear();
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
