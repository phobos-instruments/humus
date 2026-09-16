// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

inline void paintSporeCap(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fam) {
    g.setColour(fam.withAlpha(alpha::veil));
    g.fillRoundedRectangle(r.expanded(1.6f), 3.5f);
    juce::ColourGradient glow(fam.brighter(0.35f), r.getCentreX(), r.getY() + r.getHeight() * 0.22f,
                              fam.darker(0.28f), r.getCentreX(), r.getBottom(), false);
    g.setGradientFill(glow);
    g.fillRoundedRectangle(r, 2.5f);
    g.setColour(fam.brighter(0.6f).withAlpha(alpha::strong));
    g.drawRoundedRectangle(r.reduced(0.4f), 2.5f, 0.9f);
}

inline void paintSoilCell(juce::Graphics& g, juce::Rectangle<float> r) {
    g.setColour(Palette::background.darker(0.12f));
    g.fillRoundedRectangle(r, 2.5f);
    juce::ColourGradient lip(juce::Colours::black.withAlpha(alpha::muted), 0.0f, r.getY(),
                             juce::Colours::transparentBlack, 0.0f, r.getY() + 3.5f, false);
    g.setGradientFill(lip);
    g.fillRoundedRectangle(r, 2.5f);
}

}
