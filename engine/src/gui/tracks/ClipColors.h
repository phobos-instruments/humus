// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_graphics/juce_graphics.h>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

inline constexpr int kNumClipColors = ink::clip::kCount;

inline juce::Colour clipColour(int index) {
    if (index >= 1 && index <= kNumClipColors) return ink::clip::wheel[index - 1];
    return Palette::accent;
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
