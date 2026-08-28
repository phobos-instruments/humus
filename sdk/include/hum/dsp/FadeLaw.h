#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

inline float fadeGain(float x, float curve) {
    if (curve <= 0.0f) return x;
    const float p = 1.0f - 0.75f * std::min(curve, 1.0f);
    return std::pow(std::max(0.0f, x), p);
}

}
