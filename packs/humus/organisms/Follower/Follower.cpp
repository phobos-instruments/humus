#include "Follower/Follower.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace hum {

void Follower::process(const float* const* in, int numIn, float* const* out, int numOut,
                       int numSamples, const Transport&) {
    if (numOut < 1) return;
    env_.set(std::max(0.0, params.get("Attack", 10.0)),
             std::max(0.0, params.get("Release", 200.0)), sampleRate_);
    const float gain = (float) std::max(0.0, params.get("Gain", 1.0));

    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    float* dst = out[0];
    for (int i = 0; i < numSamples; ++i)
        dst[i] = (float) env_.process(src ? std::abs(src[i]) : 0.0f) * gain;
    for (int c = 1; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    if (numSamples > 0)
        ctl_.store(std::clamp(dst[numSamples - 1], 0.0f, 1.0f), std::memory_order_relaxed);
}

}
