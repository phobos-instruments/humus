// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"

namespace hum::fxlook {

inline juce::Colour bg()     { return ink::fx::ground; }
inline juce::Colour panel()  { return ink::fx::panel; }
inline juce::Colour box()    { return ink::fx::box; }
inline juce::Colour line()   { return ink::fx::line; }
inline juce::Colour text()   { return ink::fx::text; }
inline juce::Colour dim()    { return ink::fx::dim; }
inline juce::Colour accent() { return ink::fx::accent; }
inline juce::Colour warn()   { return ink::fx::warning; }

inline void styleSlider(juce::Slider& s) {
    s.setColour(juce::Slider::trackColourId, accent().withAlpha(alpha::mid));
    s.setColour(juce::Slider::thumbColourId, accent());
    s.setColour(juce::Slider::backgroundColourId, box());
    s.setColour(juce::Slider::textBoxTextColourId, text());
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

}
