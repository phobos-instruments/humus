#include "VCA/VCA.h"

#include <algorithm>
#include <cstring>

namespace hum {

void Vca::process(const float* const* in, int numIn, float* const* out, int numOut,
                  int numSamples, const Transport&) {
    if (numOut < 1) return;
    const float gain = (float) std::max(0.0, params.get("Gain", 1.0));
    const bool bipolar = params.get("Bipolar", 0.0) >= 0.5;

    const float* sig = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    const float* ctl = (numIn > 1 && in && in[1]) ? in[1] : nullptr;
    float* dst = out[0];
    for (int i = 0; i < numSamples; ++i) {
        const float s = sig ? sig[i] : 0.0f;
        float c = ctl ? ctl[i] : 1.0f;
        if (!bipolar) c = std::max(0.0f, c);
        dst[i] = s * c * gain;
    }
    for (int c = 1; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
}

}
