// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Comber/Comber.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "hum/dsp/DspMath.h"

namespace hum {

void Comber::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    const int longest = (int) (sampleRate / kLowestHz) + 4;
    for (auto& tooth : teeth_)
        for (auto& line : tooth.lines) line.prepare(longest);
    glide_ = 1.0f - (float) std::exp(-1.0 / (kGlideSeconds * sampleRate));
    reset();
}

void Comber::reset() {
    for (auto& tooth : teeth_) {
        for (auto& line : tooth.lines) line.clear();
        tooth.settled = false;
    }
}

int Comber::activeCombs() const {
    return std::clamp((int) std::lround(params.get("Combs", 4.0)) + 1, 1, kMaxCombs);
}

void Comber::aim(int count) {
    for (int k = 0; k < count; ++k) {
        auto& tooth = teeth_[(size_t) k];
        const double freq = std::max(kLowestHz, freqRef_[(size_t) k].get(params, 220.0));
        const double decay = std::max(0.02, decayRef_[(size_t) k].get(params, 1.0));
        tooth.delayGoal = (float) (sampleRate_ / freq);
        tooth.levelGoal = (float) gainRef_[(size_t) k].get(params, k == 0 ? 1.0 : 0.0);
        tooth.feedback = (float) t60Feedback(1.0 / freq, decay);
        if (tooth.settled) continue;
        tooth.delay = tooth.delayGoal;
        tooth.level = tooth.levelGoal;
        tooth.settled = true;
    }
    for (int k = count; k < kMaxCombs; ++k) {
        auto& tooth = teeth_[(size_t) k];
        if (!tooth.settled) continue;
        for (auto& line : tooth.lines) line.clear();
        tooth.settled = false;
    }
}

void Comber::process(const float* const* in, int numIn, float* const* out, int numOut,
                   int numSamples, const Transport&) {
    const int count = activeCombs();
    aim(count);
    const float master = (float) params.get("InputGain", 1.0);
    const int channels = std::min(numOut, channels_);

    for (int n = 0; n < numSamples; ++n) {
        for (int k = 0; k < count; ++k) {
            auto& tooth = teeth_[(size_t) k];
            tooth.delay += glide_ * (tooth.delayGoal - tooth.delay);
            tooth.level += glide_ * (tooth.levelGoal - tooth.level);
        }
        for (int c = 0; c < channels; ++c) {
            const int source = std::min(c, numIn - 1);
            const float dry = master * (source >= 0 && in != nullptr && in[source] != nullptr ? in[source][n] : 0.0f);
            float sum = 0.0f;
            for (int k = 0; k < count; ++k) {
                auto& tooth = teeth_[(size_t) k];
                auto& line = tooth.lines[(size_t) c];
                const float rung = line.read(tooth.delay);
                line.write(dry + tooth.feedback * rung);
                sum += tooth.level * rung;
            }
            out[c][n] = sum;
        }
    }
    for (int c = channels; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
}

}
