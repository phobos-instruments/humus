#pragma once
#include <algorithm>
#include <cmath>

#include "core/SpectrumBands.h"

namespace hum {
namespace visual {

constexpr int kBands = 8;

struct Bands {
    float level = 0.0f;
    float band[kBands] = {};
};

inline float dbToNorm(double db) {
    return (float) std::clamp((db + 60.0) / 60.0, 0.0, 1.0);
}

inline Bands fold(const float* mags, int numBins, double sampleRate,
                  const float* wave, int numWave) {
    Bands b;
    const auto db = spectrum::bandDb(mags, numBins, sampleRate);
    for (int i = 0; i < kBands; ++i) b.band[i] = dbToNorm(db[(size_t) i]);
    float peak = 0.0f;
    for (int i = 0; i < numWave; ++i) peak = std::max(peak, std::abs(wave[i]));
    b.level = std::clamp(peak, 0.0f, 1.0f);
    return b;
}

inline void smooth(Bands& state, const Bands& next, float dt, float attackSec,
                   float decaySec) {
    const auto step = [dt, attackSec, decaySec](float from, float to) {
        const float tau = to > from ? attackSec : decaySec;
        if (tau <= 1.0e-6f) return to;
        const float a = 1.0f - std::exp(-dt / tau);
        return from + (to - from) * a;
    };
    state.level = step(state.level, next.level);
    for (int i = 0; i < kBands; ++i) state.band[i] = step(state.band[i], next.band[i]);
}

}
}
