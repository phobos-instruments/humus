// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"

namespace hum {

inline void paintParamMarks(juce::Graphics& g, juce::Rectangle<float> bounds, bool controlled,
                            bool rollLocked) {
    if (controlled) {
        g.setColour(Palette::accent.withAlpha(alpha::mid));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 6.0f, 1.2f);
    }
    if (!rollLocked) return;
    const float w = 7.0f, h = 5.0f;
    const float x = bounds.getRight() - w - 3.0f, y = bounds.getY() + 5.5f;
    g.setColour(Palette::textDim.withAlpha(alpha::nearOpaque));
    juce::Path shackle;
    shackle.addCentredArc(x + w * 0.5f, y, w * 0.28f, w * 0.28f, 0.0f,
                          -juce::MathConstants<float>::halfPi,
                          juce::MathConstants<float>::halfPi, true);
    g.strokePath(shackle, juce::PathStrokeType(1.0f));
    g.fillRoundedRectangle(x, y, w, h, 1.2f);
}

}
