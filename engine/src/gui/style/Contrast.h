// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

#include <juce_graphics/juce_graphics.h>

namespace hum::contrast {

inline constexpr double kReadable = 3.0;

inline double channelLuminance(double c) {
    return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

inline double luminance(juce::Colour c) {
    return 0.2126 * channelLuminance(c.getFloatRed())
         + 0.7152 * channelLuminance(c.getFloatGreen())
         + 0.0722 * channelLuminance(c.getFloatBlue());
}

inline double ratio(juce::Colour a, juce::Colour b) {
    const double la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

inline bool isLight(juce::Colour c) { return luminance(c) > 0.34; }

inline juce::Colour readable(juce::Colour over, juce::Colour dark, juce::Colour light) {
    return ratio(over, dark) >= ratio(over, light) ? dark : light;
}

inline juce::Colour toward(juce::Colour over, juce::Colour want, bool darken, double least) {
    juce::Colour out = want;
    for (int step = 0; step < 40 && ratio(over, out) < least; ++step)
        out = darken ? out.darker(0.1f) : out.brighter(0.1f);
    return out;
}

inline juce::Colour lifted(juce::Colour over, juce::Colour want, double least = kReadable) {
    if (ratio(over, want) >= least) return want;
    const auto darker = toward(over, want, true, least);
    const auto brighter = toward(over, want, false, least);
    const double dr = ratio(over, darker), br = ratio(over, brighter);
    if (dr >= least && br >= least) return isLight(over) ? darker : brighter;
    return dr >= br ? darker : brighter;
}

}
