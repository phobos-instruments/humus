// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

inline constexpr float fadeExponent(float curve) {
    return 1.0f - 0.75f * std::min(curve, 1.0f);
}

inline float fadeGain(float x, float curve) {
    x = std::min(1.0f, std::max(0.0f, x));
    if (curve == 0.0f) return x;
    if (curve < 0.0f) return 1.0f - std::pow(1.0f - x, fadeExponent(-curve));
    return std::pow(x, fadeExponent(curve));
}

inline constexpr float kFadeLinear = 0.0f;
inline constexpr float kFadeEqualPower = 2.0f / 3.0f;
inline constexpr float kFadeLog = 1.0f;
inline constexpr float kFadeExp = -1.0f;

static_assert(fadeExponent(kFadeEqualPower) > 0.4999f
              && fadeExponent(kFadeEqualPower) < 0.5001f,
              "kFadeEqualPower must land on the square-root law");

}
