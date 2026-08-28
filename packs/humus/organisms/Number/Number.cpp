#include "Number/Number.h"

#include <cmath>
#include <cstring>

namespace hum {

void Number::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    coef_ = 1.0f - std::exp(-1.0f / (0.005f * (float) sampleRate));
    if (const auto* pv = params.byName("Value")) smoothed_ = (float) pv->value;
}

void Number::process(const float* const* in, int numIn,
                     float* const* out, int numOut,
                     int numSamples, const Transport&) {
    if (numOut < 1) return;
    const auto* pv = params.byName("Value");
    const auto* pi = params.byName("Integer");
    const float target = pv ? (float) pv->value : 0.0f;
    const bool integer = pi && pi->value >= 0.5;

    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    float* dst = out[0];
    for (int i = 0; i < numSamples; ++i) {
        smoothed_ += (target - smoothed_) * coef_;
        float v = (src ? src[i] : 0.0f) + smoothed_;
        dst[i] = integer ? std::rint(v) : v;
    }
    for (int c = 1; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
}

}
