// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/PianoRollEditor.h"

#include <memory>
#include <vector>

#include "gui/common/Localisation.h"
#include "gui/host/EngineHostClips.h"
#include "gui/pianoroll/NoteColourMenu.h"
#include "gui/tracks/QuantiseMenu.h"

namespace hum {

int PianoRollEditor::selectedNotesColour() const { return model_.selectedNotesColour(); }

void PianoRollEditor::colourSelectedNotes(int colour, bool asUndoStep) {
    model_.colourSelectedNotes(geometry(), colour, asUndoStep);
}

int PianoRollEditor::selectedCCsColour() const { return model_.selectedCCsColour(); }

void PianoRollEditor::colourSelectedCCs(int colour, bool asUndoStep) {
    model_.colourSelectedCCs(colour, asUndoStep);
    repaint();
}

void PianoRollEditor::deleteSelectedCCs() {
    model_.deleteSelectedCCs();
    repaint();
}

void PianoRollEditor::showNoteMenu(int hit) {
    if (hit >= 0 && model_.selection.count(hit) == 0) {
        model_.selection = {hit};
        repaint();
    }
    if (model_.selection.empty()) return;
    enum { kGroove = 1, kDelete };
    juce::PopupMenu m;
    const double gridBeats = snapTicks() / (double) Pattern::kTicksPerBeat;
    m.addSubMenu(tr("tracks-pane-menu.color", "Color"), notecolour::menu(selectedNotesColour()));
    m.addSubMenu(tr("tracks-pane-menu.quantise-to", "Quantise to"), quantise::menu(gridBeats));
    m.addItem(kGroove, tr("piano-roll-input.print-groove-into-selection", "Print groove into selection"));
    m.addSeparator();
    m.addItem(kDelete, tr("piano-roll-input.delete-selection", "Delete selection"));
    juce::Component::SafePointer<PianoRollEditor> self(this);
    m.showMenuAsync(juce::PopupMenu::Options(), [self, gridBeats](int r) {
        if (self == nullptr || r == 0) return;
        const auto c = notecolour::choiceFor(r);
        if (const int q = quantise::ticksFor(r, gridBeats); q > 0) self->quantiseSelection(q);
        else if (c.pick == notecolour::Pick::Colour) self->colourSelectedNotes(c.colour);
        else if (c.pick == notecolour::Pick::Custom) self->openColourPicker(false);
        else if (r == kGroove) self->printGrooveToSelection();
        else if (r == kDelete) self->deleteSelection();
    });
}

void PianoRollEditor::openColourPicker(bool ccs) {
    const int now = ccs ? selectedCCsColour() : selectedNotesColour();
    const auto mouse = getMouseXYRelative();
    const auto area = localAreaToGlobal(juce::Rectangle<int>(mouse, mouse).expanded(4));
    auto undoTaken = std::make_shared<bool>(false);
    juce::Component::SafePointer<PianoRollEditor> self(this);
    notecolour::openPicker(area, notecolour::fill(now, Palette::accent),
                           [self, undoTaken, ccs](int rgb) {
        if (self == nullptr) return;
        if (ccs) self->colourSelectedCCs(rgb, !*undoTaken);
        else self->colourSelectedNotes(rgb, !*undoTaken);
        *undoTaken = true;
    });
}

}
