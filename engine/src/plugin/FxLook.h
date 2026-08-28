#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace hum::fxlook {

inline juce::Colour bg()     { return juce::Colour(0xff17181c); }
inline juce::Colour panel()  { return juce::Colour(0xff1d1f24); }
inline juce::Colour box()    { return juce::Colour(0xff262a31); }
inline juce::Colour line()   { return juce::Colour(0xff3a404b); }
inline juce::Colour text()   { return juce::Colour(0xffd8dbe0); }
inline juce::Colour dim()    { return juce::Colour(0xff8a8f99); }
inline juce::Colour accent() { return juce::Colour(0xffa6d608); }
inline juce::Colour warn()   { return juce::Colour(0xffd6a608); }

inline void styleSlider(juce::Slider& s) {
    s.setColour(juce::Slider::trackColourId, accent().withAlpha(0.55f));
    s.setColour(juce::Slider::thumbColourId, accent());
    s.setColour(juce::Slider::backgroundColourId, box());
    s.setColour(juce::Slider::textBoxTextColourId, text());
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

}
