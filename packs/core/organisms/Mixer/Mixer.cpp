// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
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
        if (params.get(keys_[(size_t) k].solo, 0.0) >= 0.5) anySolo = true;

    for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    for (int k = 0; k < numInputs_; ++k) {
        const Keys& key = keys_[(size_t) k];
        const bool mute = params.get(key.mute, 0.0) >= 0.5;
        const bool solo = params.get(key.solo, 0.0) >= 0.5;
        const bool heard = !(mute || (anySolo && !solo));
        const float g = heard ? (float) params.get(key.gain, 1.0) * master : 0.0f;
        SmoothedGain& gL = gainL_[(size_t) k];
        SmoothedGain& gR = gainR_[(size_t) k];
        if (pan_) {
            const float p = (float) std::clamp(params.get(key.pan, 0.5), 0.0, 1.0);
            gL.setTarget(g * std::min(1.0f, 2.0f * (1.0f - p)));
            gR.setTarget(g * std::min(1.0f, 2.0f * p));
        } else {
            gL.setTarget(g);
            gR.setTarget(g);
        }
        if (gL.silent() && gR.silent()) continue;
        if (width_ == 2) {
            const int chL = k * 2, chR = k * 2 + 1;
            const float* srcL = chL < numIn ? in[chL] : nullptr;
            const float* srcR = chR < numIn ? in[chR] : nullptr;
            if (srcR == nullptr) srcR = srcL;
            if (srcL && numOut > 0) gL.applyAdd(srcL, out[0], numSamples); else gL.skip(numSamples);
            if (srcR && numOut > 1) gR.applyAdd(srcR, out[1], numSamples); else gR.skip(numSamples);
        } else if (pan_) {
            const float* src = k < numIn ? in[k] : nullptr;
            if (src && numOut > 0) gL.applyAdd(src, out[0], numSamples); else gL.skip(numSamples);
            if (src && numOut > 1) gR.applyAdd(src, out[1], numSamples); else gR.skip(numSamples);
        } else {
            if (numOut > 0 && k < numIn && in[k]) gL.applyAdd(in[k], out[0], numSamples);
            else gL.skip(numSamples);
            gR.skip(numSamples);
        }
    }
}

}
