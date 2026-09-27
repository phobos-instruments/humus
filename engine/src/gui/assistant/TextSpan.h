// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>

#include <juce_graphics/juce_graphics.h>

namespace hum::textspan {

inline juce::Range<float> rowOf(const juce::TextLayout::Line& line) {
    return {line.lineOrigin.y - line.ascent, line.lineOrigin.y + line.descent};
}

inline int charOfGlyph(const juce::TextLayout::Run& run, int glyph) {
    return run.stringRange.getStart() + std::min(glyph, std::max(0, run.stringRange.getLength() - 1));
}

inline int charAt(const juce::TextLayout& layout, juce::Point<float> at) {
    int last = 0;
    for (int i = 0; i < layout.getNumLines(); ++i) {
        const auto& line = layout.getLine(i);
        last = line.stringRange.getEnd();
        if (at.y > rowOf(line).getEnd() && i + 1 < layout.getNumLines()) continue;
        if (at.y < rowOf(line).getStart() && i == 0) return line.stringRange.getStart();
        for (const auto* run : line.runs)
            for (int g = 0; g < run->glyphs.size(); ++g) {
                const auto& glyph = run->glyphs.getReference(g);
                const float x = line.lineOrigin.x + glyph.anchor.x;
                if (at.x < x + glyph.width * 0.5f) return charOfGlyph(*run, g);
            }
        return line.stringRange.getEnd();
    }
    return last;
}

inline juce::RectangleList<float> rectsOf(const juce::TextLayout& layout, juce::Range<int> chars) {
    juce::RectangleList<float> out;
    if (chars.isEmpty()) return out;
    for (int i = 0; i < layout.getNumLines(); ++i) {
        const auto& line = layout.getLine(i);
        float from = 0.0f, to = 0.0f;
        bool any = false;
        for (const auto* run : line.runs)
            for (int g = 0; g < run->glyphs.size(); ++g) {
                if (!chars.contains(charOfGlyph(*run, g))) continue;
                const auto& glyph = run->glyphs.getReference(g);
                const float x = line.lineOrigin.x + glyph.anchor.x;
                from = any ? std::min(from, x) : x;
                to = any ? std::max(to, x + glyph.width) : x + glyph.width;
                any = true;
            }
        if (any) out.add({from, rowOf(line).getStart(), to - from, rowOf(line).getLength()});
    }
    return out;
}

}
