// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "core/midi/MidiFormat.h"
#include "core/packs/Roles.h"
#include <algorithm>
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "hum/Pattern.h"
#include "io/PatchDocument.h"

namespace hum::noteedit {

inline constexpr int kEdgePx = 4;
inline constexpr int kMinTicks = 3;

enum class Tool { Pointer, Draw, Scissors, Eraser, Line };
inline constexpr int kToolCount = 5;

inline float edgeBand(float width) {
    return juce::jlimit(2.0f, (float) kEdgePx, width * 0.25f);
}

enum class Grab { Miss, Body, LeftEdge, RightEdge };

inline Grab grabAt(juce::Rectangle<float> bounds, juce::Point<float> p) {
    if (!bounds.expanded(0.0f, 1.0f).contains(p)) return Grab::Miss;
    const float band = edgeBand(bounds.getWidth());
    if (bounds.getWidth() <= 3.0f * band) return Grab::Body;
    if (p.x <= bounds.getX() + band) return Grab::LeftEdge;
    if (p.x >= bounds.getRight() - band) return Grab::RightEdge;
    return Grab::Body;
}

inline juce::MouseCursor cursorFor(Grab g) {
    return g == Grab::LeftEdge || g == Grab::RightEdge
               ? juce::MouseCursor::LeftRightResizeCursor
               : juce::MouseCursor::NormalCursor;
}

inline juce::MouseCursor cursorForTool(Tool t);

inline const juce::MouseCursor& pencilCursor() {
    static const juce::MouseCursor cursor = [] {
        constexpr int kSize = 24;
        juce::Image img(juce::Image::ARGB, kSize, kSize, true);
        juce::Graphics g(img);
        juce::Path pen;
        pen.startNewSubPath(3.0f, 21.0f);
        pen.lineTo(6.0f, 17.0f);
        pen.lineTo(19.0f, 4.0f);
        pen.lineTo(15.0f, 2.0f);
        pen.lineTo(3.0f, 21.0f);
        pen.closeSubPath();
        g.setColour(juce::Colours::white.withAlpha(alpha::nearOpaque));
        g.strokePath(pen, juce::PathStrokeType(3.0f));
        g.setColour(juce::Colours::black.withAlpha(alpha::heavy));
        g.strokePath(pen, juce::PathStrokeType(1.4f));
        return juce::MouseCursor(img, 3, 21);
    }();
    return cursor;
}

inline juce::MouseCursor cursorForTool(Tool t) {
    switch (t) {
        case Tool::Draw:     return pencilCursor();
        case Tool::Scissors: return juce::MouseCursor::CrosshairCursor;
        case Tool::Eraser:   return juce::MouseCursor::CrosshairCursor;
        case Tool::Line:     return juce::MouseCursor::CrosshairCursor;
        default:             return juce::MouseCursor::NormalCursor;
    }
}

inline int snapDelta(int delta, int grid) {
    if (grid <= 0) return delta;
    return (int) std::lround((double) delta / (double) grid) * grid;
}

inline int octaveSteps(const PatchDocumentModel& m) {
    int steps = 12;
    for (const auto& cm : m.organisms) {
        if (!classHasRole(cm.classRaw, role::kTuning)) continue;
        double preset = 0.0, divisions = 12.0;
        for (const auto& p : cm.properties) {
            if (p.name == "Preset") preset = p.value;
            if (p.name == "Divisions") divisions = p.value;
        }
        if (preset != 0.0) return 0;
        steps = std::max(2, (int) std::lround(divisions));
    }
    return steps;
}

inline int resizeRight(int noteTick, int pointerTick, int minTicks) {
    return std::max(minTicks, pointerTick - noteTick);
}

struct HeadResize { int tick, lengthTicks; };
inline HeadResize resizeLeft(int noteTick, int noteLen, int pointerTick, int minTicks) {
    const int end = noteTick + noteLen;
    const int tick = std::clamp(pointerTick, 0, end - minTicks);
    return {tick, end - tick};
}

class WheelAccum {
public:
    int add(float delta, float gain) {
        acc_ += delta * gain;
        const int whole = (int) acc_;
        acc_ -= (float) whole;
        return whole;
    }
    void reset() { acc_ = 0.0f; }

private:
    float acc_ = 0.0f;
};

inline void paintNoteName(juce::Graphics& g, juce::Rectangle<float> box, int pitch,
                          juce::Colour ink) {
    if (box.getWidth() < 26.0f || box.getHeight() < 9.0f) return;
    g.setColour(ink);
    g.setFont(juce::FontOptions(std::min(11.0f, box.getHeight() - 1.0f)));
    g.drawText(juce::String(midiNoteName(pitch)), box.reduced(3.0f, 0.0f).toNearestInt(),
               juce::Justification::centredLeft, false);
}

inline bool isBlackKey(int pitch) {
    return ((1 << (((pitch % 12) + 12) % 12)) & 0b0000'0101'0010'1010) != 0;
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
