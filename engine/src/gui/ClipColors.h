#pragma once
#include <juce_graphics/juce_graphics.h>

#include "gui/LookAndFeel.h"

namespace hum {

inline constexpr int kNumClipColors = 8;

inline juce::Colour clipColour(int index) {
    switch (index) {
        case 1: return juce::Colour(0xffc75450);
        case 2: return juce::Colour(0xffcf8a3c);
        case 3: return juce::Colour(0xffc9b458);
        case 4: return juce::Colour(0xff6aa84f);
        case 5: return juce::Colour(0xff45a5a0);
        case 6: return juce::Colour(0xff5b8dd9);
        case 7: return juce::Colour(0xff9a6fd0);
        case 8: return juce::Colour(0xffc06fa8);
        default: return Palette::accent;
    }
}

inline juce::Colour clipFill(int index) {
    return index > 0 ? clipColour(index).darker(0.6f) : Palette::accentDim;
}

inline const char* clipColourName(int index) {
    static const char* names[] = {"Default", "Red",  "Orange", "Yellow", "Green",
                                  "Teal",    "Blue", "Purple", "Pink"};
    return index >= 0 && index <= kNumClipColors ? names[index] : "Default";
}

}
