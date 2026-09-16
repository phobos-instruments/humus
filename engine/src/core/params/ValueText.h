// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_core/juce_core.h>

namespace hum {

inline juce::String smartValueText(double v) {
    if (std::abs(v - std::round(v)) < 1e-9)
        return juce::String((juce::int64) std::llround(v));
    const double a = std::abs(v);
    const int dp = a >= 100.0 ? 1 : a >= 10.0 ? 2 : 3;
    juce::String t(v, dp);
    while (t.contains(".") && t.endsWithChar('0')) t = t.dropLastCharacters(1);
    if (t.endsWithChar('.')) t = t.dropLastCharacters(1);
    return t;
}

}
