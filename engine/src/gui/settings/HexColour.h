// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <optional>

#include <juce_graphics/juce_graphics.h>

namespace hum {

inline std::optional<juce::Colour> parseHexColour(const juce::String& typed) {
    auto digits = typed.trim();
    if (digits.startsWithChar('#')) digits = digits.substring(1);
    if (!digits.containsOnly("0123456789abcdefABCDEF")) return std::nullopt;
    if (digits.length() == 3)
        digits = juce::String::charToString(digits[0]) + digits[0] + digits[1] + digits[1]
                 + digits[2] + digits[2];
    if (digits.length() != 6) return std::nullopt;
    return juce::Colour((juce::uint32) (0xff000000u | (juce::uint32) digits.getHexValue32()));
}

inline juce::String hexText(juce::Colour c) { return "#" + c.toDisplayString(false); }

}
