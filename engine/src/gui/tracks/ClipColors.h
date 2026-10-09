// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_graphics/juce_graphics.h>

#include "gui/common/Localisation.h"
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

inline juce::String clipColourName(int index) {
    struct Name { const char* key; const char* text; };
    static const Name names[] = {
        {"clip-colour.default", "Default"}, {"clip-colour.red", "Red"},       {"clip-colour.orange", "Orange"},
        {"clip-colour.yellow", "Yellow"},   {"clip-colour.green", "Green"},   {"clip-colour.teal", "Teal"},
        {"clip-colour.blue", "Blue"},       {"clip-colour.purple", "Purple"}, {"clip-colour.pink", "Pink"}};
    const auto& name = names[index >= 0 && index <= kNumClipColors ? index : 0];
    return tr(name.key, name.text);
}

}
