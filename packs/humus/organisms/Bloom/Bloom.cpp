// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Bloom/Bloom.h"

#include <algorithm>

namespace hum {

void Bloom::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport&) {
    if (numOut == 0) return;

    sv::EngineParams p;
    p.mix = (float) params.get("Mix", 0.5);
    p.shiftSemitones = (float) params.get("Shift", 12.0);
    p.feedback = (float) params.get("Feedback", 0.5);
    p.diffusion = (float) params.get("Diffusion", 0.7);
    p.size = (float) params.get("Size", 0.5);
    p.lowCutHz = (float) params.get("LowCut", 100.0);
    p.highCutHz = (float) params.get("HighCut", 8000.0);
    p.modRateHz = (float) params.get("ModRate", 0.5);
    p.modDepth = (float) params.get("ModDepth", 0.25);
    p.shimmer = (float) params.get("Shimmer", 0.6);
    auto mode = [&](const char* name, double def, int hi) {
        return std::clamp((int) (params.get(name, def) + 0.5), 0, hi);
    };
    p.reverbMode = (sv::ReverbMode) mode("Mode", 1.0, 3);
    p.shiftMode = (sv::ShiftMode) mode("Voice", 0.0, 3);
    p.colorMode = (sv::ColorMode) mode("Color", 1.0, 2);
    engine_.setParams(p);

    const float* l = numIn > 0 && in ? in[0] : nullptr;
    const float* r = numIn > 1 && in && in[1] ? in[1] : l;
    const float* src[2] = {l, r};
    for (int c = 0; c < std::min(numOut, 2); ++c) {
        if (src[c]) std::copy(src[c], src[c] + numSamples, out[c]);
        else std::fill(out[c], out[c] + numSamples, 0.0f);
    }
    for (int c = 2; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
    engine_.process(out[0], numOut > 1 ? out[1] : out[0], numSamples);
}

}
