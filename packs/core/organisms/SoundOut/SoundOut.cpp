// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SoundOut/SoundOut.h"

#include <algorithm>

namespace hum {

void SoundOut::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    meter_.prepare(sampleRate);
    gainSm_ = (float) params.get("Gain", 1.0);
    block_.assign((size_t) channels_, std::vector<float>((size_t) maxBlock, 0.0f));
    dc_.assign((size_t) channels_, DcBlock{});
    for (auto& d : dc_) d.prepare(sampleRate, 5.0);
    blockLen_ = 0;
}

void SoundOut::process(const float* const* in, int numIn,
                       float* const*, int,
                       int numSamples, const Transport&) {
    blockLen_ = numSamples;
    const float target = (float) std::clamp(params.get("Gain", 1.0), 0.0, 2.0);
    const float step = (target - gainSm_) / (float) std::max(1, numSamples);
    const bool guard = params.get("DcGuard", 1.0) >= 0.5;
    const float* meterPtrs[LevelMeter::kMax];
    const int mCh = std::min(channels_, LevelMeter::kMax);
    for (int c = 0; c < channels_; ++c) {
        float* d = block_[(size_t) c].data();
        if (c < numIn && in[c]) {
            float g = gainSm_;
            for (int n = 0; n < numSamples; ++n) { g += step; d[n] = in[c][n] * g; }
            if (guard && (size_t) c < dc_.size()) dc_[(size_t) c].process(d, d, numSamples);
        } else {
            std::fill(d, d + numSamples, 0.0f);
        }
        if (c < mCh) meterPtrs[c] = d;
    }
    gainSm_ = target;
    meter_.measure(meterPtrs, mCh, numSamples);
}

}
