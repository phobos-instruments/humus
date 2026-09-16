// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/pianoroll/NoteEdit.h"
#include "gui/style/LookAndFeel.h"

namespace hum::timelinechrome {

inline void paintToolIcon(juce::Graphics& g, juce::Rectangle<float> r,
                          noteedit::Tool t) {
    if (t == noteedit::Tool::Pointer) {
        juce::Path a;
        a.startNewSubPath(r.getX() + 1.0f, r.getY());
        a.lineTo(r.getX() + 1.0f, r.getBottom() - 1.0f);
        a.lineTo(r.getX() + 4.0f, r.getBottom() - 4.0f);
        a.lineTo(r.getRight() - 2.0f, r.getBottom() + 1.0f);
        g.strokePath(a, juce::PathStrokeType(1.4f));
    } else if (t == noteedit::Tool::Draw) {
        juce::Path pen;
        pen.startNewSubPath(r.getX() + 1.0f, r.getBottom());
        pen.lineTo(r.getX() + 3.0f, r.getBottom() - 3.0f);
        pen.lineTo(r.getRight(), r.getY() + 1.0f);
        pen.lineTo(r.getRight() - 3.0f, r.getY() - 1.0f);
        pen.lineTo(r.getX() + 1.0f, r.getBottom());
        g.strokePath(pen, juce::PathStrokeType(1.3f));
    } else if (t == noteedit::Tool::Line) {
        g.drawLine(r.getX() + 1.0f, r.getBottom() - 1.0f, r.getRight() - 1.0f, r.getY() + 1.0f, 1.4f);
        g.fillEllipse(r.getX() - 1.0f, r.getBottom() - 3.0f, 4.0f, 4.0f);
        g.fillEllipse(r.getRight() - 3.0f, r.getY() - 1.0f, 4.0f, 4.0f);
    } else if (t == noteedit::Tool::Scissors) {
        g.drawLine(r.getX(), r.getY(), r.getRight(), r.getBottom(), 1.4f);
        g.drawLine(r.getRight(), r.getY(), r.getX(), r.getBottom(), 1.4f);
        g.fillEllipse(r.getX() - 1.0f, r.getBottom() - 2.0f, 4.0f, 4.0f);
        g.fillEllipse(r.getRight() - 3.0f, r.getBottom() - 2.0f, 4.0f, 4.0f);
    } else {
        juce::Path er;
        er.addRoundedRectangle(r.getX(), r.getCentreY() - 3.0f, r.getWidth(), 7.0f, 2.0f);
        g.strokePath(er, juce::PathStrokeType(1.3f),
                     juce::AffineTransform::rotation(-0.5f, r.getCentreX(), r.getCentreY()));
    }
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
    g.setColour(juce::Colours::white.withAlpha(alpha::nearOpaque));
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0) paintToolIcon(g, r.translated((float) dx, (float) dy), t);
    g.setColour(juce::Colours::black.withAlpha(alpha::nearOpaque));
    paintToolIcon(g, r, t);
    const bool tip = t == noteedit::Tool::Draw || t == noteedit::Tool::Line;
    return cache.emplace((int) t, juce::MouseCursor(img, tip ? 6 : kSize / 2, tip ? 20 : kSize / 2))
        .first->second;
}

}
