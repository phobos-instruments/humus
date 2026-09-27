// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TrackRollView.h"

#include <memory>
#include <vector>

#include "gui/common/Localisation.h"
#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteColourMenu.h"
#include "gui/tracks/QuantiseMenu.h"

namespace hum {

void TrackRollView::showNoteMenu(juce::Point<int> screenPos) {
    enum { kNoteDelete = 1, kNoteLouder, kNoteSofter, kNoteSelectAll };
    juce::PopupMenu m;
    const int n = (int) sel_.size();
    const auto count = n == 1 ? juce::String("note") : juce::String(n) + " notes";
    m.addSectionHeader(n > 0 ? count : juce::String(tr("tracks-pane-menu.no-notes-selected", "No notes selected")));
    m.addItem(kNoteSelectAll, tr("tracks-pane-menu.select-all", "Select All"), true);
    if (n > 0) {
        m.addSeparator();
        m.addSubMenu(tr("tracks-pane-menu.quantise-to", "Quantise to"), quantise::menu(ctx_.gridBeats()));
        m.addSeparator();
        m.addItem(kNoteLouder, tr("tracks-pane-menu.louder", "Louder"));
        m.addItem(kNoteSofter, tr("tracks-pane-menu.softer", "Softer"));
        m.addSeparator();
        m.addSubMenu(tr("tracks-pane-menu.color", "Color"),
                     notecolour::menu(selectedNotesColour()));
        m.addSeparator();
        m.addItem(kNoteDelete, tr("tracks-pane-menu.delete", "Delete"));
    }
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos, screenPos}),
                    [this, screenPos](int res) {
        if (res == 0) return;
        if (const auto c = notecolour::choiceFor(res); c.pick == notecolour::Pick::Colour) {
            colourSelectedNotes(c.colour);
            return;
        } else if (c.pick == notecolour::Pick::Custom) {
            openNoteColourPicker(screenPos);
            return;
        }
        if (res == kNoteSelectAll) { selectAllNotes(); return; }
        if (const int q = quantise::ticksFor(res, ctx_.gridBeats()); q > 0) {
            quantiseSelectedNotes(q);
            return;
        }
        if (res == kNoteDelete) { deleteSelectedNotes(); ctx_.rebuildRows(); return; }
        if (res == kNoteLouder) nudgeNotes(0, 0, 10);
        if (res == kNoteSofter) nudgeNotes(0, 0, -10);
    });
}

int TrackRollView::selectedNotesColour() const {
    std::vector<int> colours;
    for (const auto& ci : host().clips().list(node_)) {
        const auto notes = host().clips().notes(node_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i)
            if (sel_.count({ci.index, i}) != 0) colours.push_back(notes[(size_t) i].colour);
    }
    return notecolour::shared(colours);
}

void TrackRollView::colourSelectedNotes(int colour, bool asUndoStep) {
    if (sel_.empty()) return;
    if (asUndoStep) host().pushUndo();
    for (const auto& ci : host().clips().list(node_)) {
        auto notes = host().clips().notes(node_, ci.index);
        bool changed = false;
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (sel_.count({ci.index, i}) == 0 || notes[(size_t) i].colour == colour) continue;
            notes[(size_t) i].colour = colour;
            changed = true;
        }
        if (changed) host().clips().setNotes(node_, ci.index, notes, 0);
    }
    repaint();
}

void TrackRollView::openNoteColourPicker(juce::Point<int> screenPos) {
    const auto start = notecolour::fill(selectedNotesColour(), Palette::accent);
    auto undoTaken = std::make_shared<bool>(false);
    juce::Component::SafePointer<TrackRollView> self(this);
    notecolour::openPicker({screenPos.x, screenPos.y, 1, 1}, start,
                           [self, undoTaken](int rgb) {
        if (self == nullptr) return;
        self->colourSelectedNotes(rgb, !*undoTaken);
        *undoTaken = true;
    });
}

}
