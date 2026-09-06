#include "Slider/Slider.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace hum {

namespace {
float mapped(float value, float lo, float hi, bool log) {
    if (log && lo > 0.0f && hi > 0.0f) return lo * std::pow(hi / lo, value);
    return lo + value * (hi - lo);
}
}

void Slider::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    smoothed_ = mapped((float) std::clamp(params.get("Value", 0.0), 0.0, 1.0),
                       (float) params.get("Min", 0.0), (float) params.get("Max", 1.0),
                       params.get("Log", 0.0) >= 0.5);
}

void Slider::process(const float* const* in, int numIn,
                     float* const* out, int numOut,
                     int numSamples, const Transport&) {
    if (numOut < 1) return;
    const float value = (float) std::clamp(params.get("Value", 0.0), 0.0, 1.0);
    const float lo = (float) params.get("Min", 0.0);
    const float hi = (float) params.get("Max", 1.0);
    const float target = mapped(value, lo, hi, params.get("Log", 0.0) >= 0.5);
    const double slewS = std::max(0.0, params.get("Slew", 5.0)) * 0.001;
    const float coef = slewS <= 0.0
        ? 1.0f
        : 1.0f - (float) std::exp(-1.0 / (slewS * sampleRate_));

    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    float* dst = out[0];
    for (int i = 0; i < numSamples; ++i) {
        smoothed_ += (target - smoothed_) * coef;
        dst[i] = (src ? src[i] : 0.0f) + smoothed_;
    }
    for (int c = 1; c < numOut; ++c)
        std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    ctl_.store(value, std::memory_order_relaxed);
}

}
