// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <numeric>

#include <juce_graphics/juce_graphics.h>

#include "gui/style/Colours.h"

namespace hum {

inline constexpr int kOriginWheelStride = 3;
static_assert(std::gcd(kOriginWheelStride, ink::clip::kCount) == 1,
              "a stride sharing a factor with the wheel gives two sources the same colour");

inline juce::Colour originColour(int origin) {
    const int n = ink::clip::kCount;
    return ink::clip::wheel[(((origin % n) + n) % n * kOriginWheelStride) % n];
}

inline juce::Colour originBand(int origin) {
    return originColour(origin).withMultipliedSaturation(0.7f).withMultipliedBrightness(0.38f);
}

}
