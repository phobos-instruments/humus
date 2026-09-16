// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

inline void drawMatchLit(juce::Graphics& g, const juce::Font& font,
                         const juce::String& text, const juce::String& query,
                         juce::Rectangle<int> area,
                         juce::Colour normal, juce::Colour lit) {
    g.setFont(font);
    int x = area.getX();
    auto draw = [&](const juce::String& part, juce::Colour col) {
        if (part.isEmpty()) return;
        g.setColour(col);
        const int pw = (int) std::ceil(juce::GlyphArrangement::getStringWidth(font, part));
        g.drawText(part, x, area.getY(), juce::jmax(1, area.getRight() - x),
                   area.getHeight(), juce::Justification::centredLeft);
        x += pw;
    };
    const int at = query.isNotEmpty() ? text.toLowerCase().indexOf(query.toLowerCase()) : -1;
    if (at >= 0) {
        draw(text.substring(0, at), normal);
        draw(text.substring(at, at + query.length()), lit);
        draw(text.substring(at + query.length()), normal);
    } else {
        draw(text, normal);
    }
}

}
