#include "Lumen/Lumen.h"

namespace hum {

void Lumen::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport&) {
    (void) out; (void) numOut;
    if (!tap_.load(std::memory_order_relaxed) || numIn < 1 || in == nullptr) return;

    const float norm = 1.0f / (float) numIn;
    int w = wpos_.load(std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i) {
        float s = 0.0f;
        for (int c = 0; c < numIn; ++c)
            if (in[c] != nullptr) s += in[c][i];
        ring_[(size_t) w] = s * norm;
        if (++w >= kRing) w = 0;
    }
    wpos_.store(w, std::memory_order_relaxed);
}

int Lumen::readVisualTap(float* dest, int maxSamples) const {
    if (!tap_.load(std::memory_order_relaxed)) return 0;
    const int n = maxSamples < kRing ? maxSamples : kRing;
    int r = wpos_.load(std::memory_order_relaxed) - n;
    if (r < 0) r += kRing;
    for (int i = 0; i < n; ++i) {
        dest[i] = ring_[(size_t) r];
        if (++r >= kRing) r = 0;
    }
    return n;
}

}
