// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/pianoroll/NoteEdit.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"

namespace hum::timelinechrome {

inline IconGlyph toolGlyph(noteedit::Tool t) {
    switch (t) {
        case noteedit::Tool::Pointer:  return IconGlyph::Pointer;
        case noteedit::Tool::Draw:     return IconGlyph::Pencil;
        case noteedit::Tool::Line:     return IconGlyph::Line;
        case noteedit::Tool::Scissors: return IconGlyph::Scissors;
        default:                       return IconGlyph::Eraser;
    }
}

inline void paintToolIcon(juce::Graphics& g, juce::Rectangle<float> r, noteedit::Tool t,
                          juce::Colour ink) {
    drawIconGlyph(g, toolGlyph(t), r, ink, true);
}

inline void paintCutLine(juce::Graphics& g, float x, float top, float bottom) {
    g.setColour(Palette::text.withAlpha(alpha::strong));
    const float dash[] = {3.0f, 3.0f};
    g.drawDashedLine(juce::Line<float>(x, top, x, bottom), dash, 2, 1.0f);
}

inline const juce::MouseCursor& toolCursor(noteedit::Tool t) {
    static std::map<int, juce::MouseCursor> cache;
    auto it = cache.find((int) t);
    if (it != cache.end()) return it->second;
    if (t == noteedit::Tool::Pointer)
        return cache.emplace((int) t, juce::MouseCursor::NormalCursor).first->second;
    constexpr int kSize = 26;
    juce::Image img(juce::Image::ARGB, kSize, kSize, true);
    juce::Graphics g(img);
    const juce::Rectangle<float> r(6.0f, 6.0f, 14.0f, 14.0f);
    const auto halo = juce::Colours::white.withAlpha(alpha::nearOpaque);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0) paintToolIcon(g, r.translated((float) dx, (float) dy), t, halo);
    paintToolIcon(g, r, t, juce::Colours::black.withAlpha(alpha::nearOpaque));
    const bool tip = t == noteedit::Tool::Draw || t == noteedit::Tool::Line;
    return cache.emplace((int) t, juce::MouseCursor(img, tip ? 6 : kSize / 2, tip ? 20 : kSize / 2))
        .first->second;
}

}
