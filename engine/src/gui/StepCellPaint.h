#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

namespace hum {

inline void paintSporeCap(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fam) {
    g.setColour(fam.withAlpha(0.16f));
    g.fillRoundedRectangle(r.expanded(1.6f), 3.5f);
    juce::ColourGradient glow(fam.brighter(0.35f), r.getCentreX(), r.getY() + r.getHeight() * 0.22f,
                              fam.darker(0.28f), r.getCentreX(), r.getBottom(), false);
    g.setGradientFill(glow);
    g.fillRoundedRectangle(r, 2.5f);
    g.setColour(fam.brighter(0.6f).withAlpha(0.75f));
    g.drawRoundedRectangle(r.reduced(0.4f), 2.5f, 0.9f);
}

inline void paintSoilCell(juce::Graphics& g, juce::Rectangle<float> r) {
    g.setColour(Palette::background.darker(0.12f));
    g.fillRoundedRectangle(r, 2.5f);
    juce::ColourGradient lip(juce::Colours::black.withAlpha(0.35f), 0.0f, r.getY(),
                             juce::Colours::transparentBlack, 0.0f, r.getY() + 3.5f, false);
    g.setGradientFill(lip);
    g.fillRoundedRectangle(r, 2.5f);
}

}
