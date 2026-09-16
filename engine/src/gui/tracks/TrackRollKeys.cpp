// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TrackRollView.h"

#include <algorithm>
#include <cmath>

#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteEdit.h"
#include "hum/Pattern.h"

namespace hum {

bool TrackRollView::keyPressed(const juce::KeyPress& k) {
    if (k.getKeyCode() == juce::KeyPress::escapeKey && hasNoteSelection()) {
        clearNoteSelection();
        return true;
    }
    if (k.getKeyCode() == juce::KeyPress::escapeKey || k.getKeyCode() == juce::KeyPress::returnKey) {
        ctx_.leaveTrack();
        return true;
    }
    const auto alt = juce::ModifierKeys::altModifier;
    if (k == juce::KeyPress(juce::KeyPress::upKey, alt, 0))   { ctx_.stepTrackBy(-1); return true; }
    if (k == juce::KeyPress(juce::KeyPress::downKey, alt, 0)) { ctx_.stepTrackBy(+1); return true; }
    const auto cmd = juce::ModifierKeys::commandModifier;
    const auto shift = juce::ModifierKeys::shiftModifier;
    const int grid = std::max(1, (int) std::llround(ctx_.gridBeats() * Pattern::kTicksPerBeat));
    if (k == juce::KeyPress(juce::KeyPress::upKey, cmd, 0))    { nudgeNotes(0, 0, 10);  return true; }
    if (k == juce::KeyPress(juce::KeyPress::downKey, cmd, 0))  { nudgeNotes(0, 0, -10); return true; }
    const int oct = juce::jmax(1, noteedit::octaveSteps(host().model()));
    if (k == juce::KeyPress(juce::KeyPress::upKey, shift, 0))  { nudgeNotes(0, oct, 0);  return true; }
    if (k == juce::KeyPress(juce::KeyPress::downKey, shift, 0)){ nudgeNotes(0, -oct, 0); return true; }
    if (k == juce::KeyPress(juce::KeyPress::upKey))            { nudgeNotes(0, 1, 0);   return true; }
    if (k == juce::KeyPress(juce::KeyPress::downKey))          { nudgeNotes(0, -1, 0);  return true; }
    if (k == juce::KeyPress(juce::KeyPress::leftKey))          { nudgeNotes(-grid, 0, 0); return true; }
    if (k == juce::KeyPress(juce::KeyPress::rightKey))         { nudgeNotes(grid, 0, 0);  return true; }
    if (k == juce::KeyPress('a', cmd, 0)) { selectAllNotes(); return true; }
    if (k == juce::KeyPress('c', cmd, 0)) { copySelectedNotes(); return true; }
    if (k == juce::KeyPress('x', cmd, 0)) {
        copySelectedNotes();
        deleteSelectedNotes();
        return true;
    }
    if (k == juce::KeyPress('d', cmd, 0)) return duplicateSelectedNotes();
    if (k == juce::KeyPress('v', cmd, 0))
        return pasteNotes((int) std::llround(ctx_.snapBeats(host().positionBeats(), false) * Pattern::kTicksPerBeat));
    if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey) {
        deleteSelectedNotes();
        return true;
    }
    return false;
}

}
