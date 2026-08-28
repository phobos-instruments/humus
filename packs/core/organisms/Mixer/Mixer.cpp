#include "Mixer/Mixer.h"

#include <algorithm>

namespace hum {

std::string Mixer::inputSuffix(int k) const {
    if (width_ == 2) {
        int lo = k * 2 + 1, hi = lo + 1;
        return std::to_string(lo) + "-" + std::to_string(hi);
    }
    return std::to_string(k + 1);
}

void Mixer::process(const float* const* in, int numIn,
                    float* const* out, int numOut,
                    int numSamples, const Transport&) {
    meter_.measure(in, numIn, numSamples);
    const bool masterMute = params.get("MasterMute", 0.0) >= 0.5;
    const float master = masterMute ? 0.0f : (float) params.get("MasterGain", 1.0);

    bool anySolo = false;
    for (int k = 0; k < numInputs_; ++k)
        if (params.get("Solo_" + inputSuffix(k), 0.0) >= 0.5) anySolo = true;

    for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    for (int k = 0; k < numInputs_; ++k) {
        const std::string sfx = inputSuffix(k);
        const bool mute = params.get("Mute_" + sfx, 0.0) >= 0.5;
        const bool solo = params.get("Solo_" + sfx, 0.0) >= 0.5;
        if (mute || (anySolo && !solo)) continue;
        const float g = (float) params.get("Gain_" + sfx, 1.0) * master;
        if (width_ == 2) {
            const int chL = k * 2, chR = k * 2 + 1;
            const float* srcL = chL < numIn ? in[chL] : nullptr;
            const float* srcR = chR < numIn ? in[chR] : nullptr;
            if (srcR == nullptr) srcR = srcL;
            if (srcL && numOut > 0)
                for (int n = 0; n < numSamples; ++n) out[0][n] += g * srcL[n];
            if (srcR && numOut > 1)
                for (int n = 0; n < numSamples; ++n) out[1][n] += g * srcR[n];
        } else if (pan_) {
            const float* src = k < numIn ? in[k] : nullptr;
            if (!src) continue;
            const float p =
                (float) std::clamp(params.get("Pan_" + sfx, 0.5), 0.0, 1.0);
            const float gL = g * std::min(1.0f, 2.0f * (1.0f - p));
            const float gR = g * std::min(1.0f, 2.0f * p);
            if (numOut > 0)
                for (int n = 0; n < numSamples; ++n) out[0][n] += gL * src[n];
            if (numOut > 1)
                for (int n = 0; n < numSamples; ++n) out[1][n] += gR * src[n];
        } else if (numOut > 0 && k < numIn && in[k]) {
            for (int n = 0; n < numSamples; ++n) out[0][n] += g * in[k][n];
        }
    }
}

}
