// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "core/midi/MidiFormat.h"
#include "core/packs/Roles.h"
#include <algorithm>
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/juce/JuceGeometry.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/pianoroll/NoteRules.h"
#include "hum/Pattern.h"
#include "io/PatchDocument.h"

namespace hum::noteedit {

inline Grab grabAt(juce::Rectangle<float> bounds, juce::Point<float> p) { return grabAt(rectOf(bounds), p.x, p.y); }

inline juce::MouseCursor cursorFor(Grab g) {
    return g == Grab::LeftEdge || g == Grab::RightEdge
               ? juce::MouseCursor::LeftRightResizeCursor
               : juce::MouseCursor::NormalCursor;
}

inline void paintNoteName(juce::Graphics& g, juce::Rectangle<float> box, int pitch,
                          juce::Colour ink) {
    if (box.getWidth() < 26.0f || box.getHeight() < 9.0f) return;
    g.setColour(ink);
    g.setFont(juce::FontOptions(std::min(11.0f, box.getHeight() - 1.0f)));
    g.drawText(juce::String(midiNoteName(pitch)), box.reduced(3.0f, 0.0f).toNearestInt(),
               juce::Justification::centredLeft, false);
}

inline constexpr juce::uint32 kIvory = 0xfff2f2f2, kEbony = 0xff141414;

inline float laneRuleAlpha(int pitch) {
    const int semis = ((pitch % 12) + 12) % 12;
    return semis == 0 ? 0.75f : semis == 5 ? 0.45f : 0.16f;
}

inline void paintPianoKey(juce::Graphics& g, juce::Rectangle<float> row, int pitch,
                          bool held, juce::Colour accent) {
    const float blackW = std::floor(row.getWidth() * 0.62f);
    const bool black = isBlackKey(pitch);
    g.setColour(held && !black ? accent : juce::Colour(kIvory));
    g.fillRect(row);
    if (black) {
        auto key = row.withWidth(blackW);
        g.setColour(held ? accent.darker(0.5f) : juce::Colour(kEbony));
        g.fillRect(key);
        g.setColour(Palette::border);
        g.drawRect(key, 1.0f);
    }
    const float x = black ? row.getX() + blackW : row.getX();
    g.setColour(juce::Colour(kEbony).withAlpha(laneRuleAlpha(pitch)));
    g.fillRect(x, row.getBottom() - 1.0f, row.getRight() - x, 1.0f);
}

}
