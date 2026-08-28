#include "Spectrum/Spectrum.h"

#include <algorithm>
#include <cmath>

namespace hum {

void Spectrum::process(const float* const* in, int numIn, float* const* out, int numOut,
                       int numSamples, const Transport&) {
    const float* l = numIn > 0 && in ? in[0] : nullptr;
    const float* r = numIn > 1 && in && in[1] ? in[1] : l;
    const float* src[2] = {l, r};
    for (int c = 0; c < numOut; ++c) {
        const float* s = c < 2 ? src[c] : nullptr;
        if (s) std::copy(s, s + numSamples, out[c]);
        else std::fill(out[c], out[c] + numSamples, 0.0f);
    }
    int w = write_.load(std::memory_order_relaxed);
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        const float a = l ? l[i] : 0.0f;
        const float b = r ? r[i] : 0.0f;
        const float m = 0.5f * (a + b);
        ring_[(size_t) w] = m;
        w = (w + 1) % kScopeSamples;
        peak = std::max(peak, std::abs(m));
    }
    write_.store(w, std::memory_order_relaxed);
    if (peak > 1.0e-6f) stamp_.fetch_add(1, std::memory_order_relaxed);
}

}
