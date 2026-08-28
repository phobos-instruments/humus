#pragma once
#include <cmath>

#include "hum/dsp/WaveTable.h"

namespace hum {

inline float warpRead(const float* tab, int len, double phase, int mode, float amt) {
    if (mode <= 0 || amt <= 0.0f) return waveTableRead(tab, len, phase);
    switch (mode) {
        case 1: return waveTableRead(tab, len, phase * (1.0 + amt * 3.0));
        case 2: return waveTableRead(tab, len, std::pow(phase, 1.0 + amt * 3.0));
        case 3: {
            float x = waveTableRead(tab, len, phase) * (1.0f + amt * 5.0f);
            for (int k = 0; k < 6 && (x > 1.0f || x < -1.0f); ++k)
                x = x > 1.0f ? 2.0f - x : -2.0f - x;
            return x;
        }
        case 4: {
            const double width = 0.5 - amt * 0.45;
            const double p = phase < width ? 0.5 * phase / width
                                           : 0.5 + 0.5 * (phase - width) / (1.0 - width);
            return waveTableRead(tab, len, p);
        }
        default: return waveTableRead(tab, len, phase);
    }
}

}
