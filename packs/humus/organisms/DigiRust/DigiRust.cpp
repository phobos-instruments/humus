#include "DigiRust/DigiRust.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum {

void DigiRust::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    for (auto& d : dc_) d.prepare(sampleRate);
    env_.set(5.0, 80.0, sampleRate);
    reset();
}

void DigiRust::reset() {
    phase_ = 0.0;
    period_ = 1.0;
    hold_[0] = hold_[1] = 0.0f;
    tone_[0] = tone_[1] = 0.0f;
    env_.reset();
    rng_ = 0xD161D05u;
}

void DigiRust::process(const float* const* in, int numIn, float* const* out, int numOut,
                       int numSamples, const Transport&) {
    if (numOut == 0) return;
    const float bits = (float) std::clamp(params.get("Bits", 8.0), 1.0, 16.0);
    const double rate = std::clamp(params.get("Rate", 12000.0), 500.0, 48000.0);
    const float jitter = (float) std::clamp(params.get("Jitter", 0.0), 0.0, 1.0);
    const float noise = (float) std::clamp(params.get("Noise", 0.0), 0.0, 1.0);
    const float tone = (float) std::clamp(params.get("Tone", 0.5), 0.0, 1.0);
    const float mix = (float) std::clamp(params.get("Mix", 1.0), 0.0, 1.0);
    const float level = (float) std::clamp(params.get("Level", 1.0), 0.0, 2.0);

    const float steps = std::exp2(bits - 1.0f);
    const double basePeriod = std::max(1.0, sampleRate_ / rate);
    const bool decimate = basePeriod > 1.0 + 1e-9;
    const float tc = (float) (1.0 - std::exp(-2.0 * kPi * 1600.0 / sampleRate_));
    const float tilt = (tone - 0.5f) * 2.0f;

    float* L = out[0];
    float* R = numOut > 1 ? out[1] : nullptr;
    for (int i = 0; i < numSamples; ++i) {
        const float xin[2] = {
            numIn > 0 && in[0] ? in[0][i] : 0.0f,
            numIn > 1 && in[1] ? in[1][i] : (numIn > 0 && in[0] ? in[0][i] : 0.0f)};

        phase_ += 1.0;
        const bool grab = !decimate || phase_ >= period_;
        if (grab && decimate) {
            phase_ -= period_;
            period_ = basePeriod * (1.0 + (frand() * 2.0f - 1.0f) * jitter * 0.6);
            period_ = std::max(1.0, period_);
        }
        const double loud = env_.process(0.5 * (std::abs(xin[0]) + std::abs(xin[1])));
        const float hiss = noise > 0.0f
                               ? (frand() * 2.0f - 1.0f) * noise * noise * 0.5f * (float) loud
                               : 0.0f;

        float wet[2];
        for (int c = 0; c < 2; ++c) {
            if (grab) hold_[c] = std::round(xin[c] * steps) / steps;
            float y = hold_[c] + hiss;
            tone_[c] += tc * (y - tone_[c]);
            y = tilt < 0.0f ? y + (-tilt) * (tone_[c] - y)
                            : y + tilt * 1.5f * (y - tone_[c]);
            wet[c] = y;
        }
        L[i] = xin[0] + mix * (wet[0] - xin[0]);
        if (R != nullptr) R[i] = xin[1] + mix * (wet[1] - xin[1]);
    }
    dc_[0].process(L, L, numSamples);
    if (R != nullptr) dc_[1].process(R, R, numSamples);
    if (level != 1.0f) {
        for (int i = 0; i < numSamples; ++i) L[i] *= level;
        if (R != nullptr)
            for (int i = 0; i < numSamples; ++i) R[i] *= level;
    }
    for (int c = 2; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
}

}
