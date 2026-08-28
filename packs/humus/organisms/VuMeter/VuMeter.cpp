#include "VuMeter/VuMeter.h"

#include <cmath>

namespace hum {

void VuMeter::process(const float* const* in, int numIn,
                      float* const* out, int numOut,
                      int numSamples, const Transport&) {
    const float* l = numIn > 0 ? in[0] : nullptr;
    const float* r = numIn > 1 && in[1] != nullptr ? in[1] : l;
    const float* src[2] = {l, r};
    for (int c = 0; c < 2; ++c) {
        float sumSq = 0.0f, pk = 0.0f;
        if (const float* s = src[c]) {
            for (int n = 0; n < numSamples; ++n) {
                const float x = s[n];
                sumSq += x * x;
                const float a = std::abs(x);
                if (a > pk) pk = a;
            }
        }
        rms_[c].store(numSamples > 0 ? std::sqrt(sumSq / (float) numSamples)
                                     : 0.0f,
                      std::memory_order_relaxed);
        peak_[c].store(pk, std::memory_order_relaxed);
        if (c < numOut && out[c] != nullptr) {
            const float* s = src[c];
            for (int n = 0; n < numSamples; ++n) out[c][n] = s ? s[n] : 0.0f;
        }
    }
}

}
