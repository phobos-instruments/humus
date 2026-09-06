#include "Drops/Drops.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void Drops::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    const double sr = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
    poolBufL_.assign((size_t) std::max(16.0, sr * 0.0071), 0.0f);
    poolBufR_.assign((size_t) std::max(16.0, sr * 0.0097), 0.0f);
    reset();
}

void Drops::reset() {
    for (auto& v : voices_) v.t = -1;
    std::fill(poolBufL_.begin(), poolBufL_.end(), 0.0f);
    std::fill(poolBufR_.begin(), poolBufR_.end(), 0.0f);
    poolPosL_ = poolPosR_ = 0;
    poolLpL_ = poolLpR_ = 0.0f;
    nextDrop_ = 0.0;
    rng_ = 0x5EEDD09u;
}

void Drops::trigger(double size, double spread, double chirp, float width) {
    Voice* v = nullptr;
    for (auto& c : voices_)
        if (c.t < 0) { v = &c; break; }
    if (v == nullptr) return;

    const double s = std::clamp(size + (frand() * 2.0 - 1.0) * spread * 0.5, 0.0, 1.0);
    v->f0 = 2400.0 * std::pow(0.12, s);
    v->damp = 0.13 * v->f0 + 0.0072 * std::pow(v->f0, 1.5);
    v->sigma = chirp * 0.25 * v->damp;
    v->amp = (float) (0.25 + 0.75 * s);
    const float pan = 0.5f + (frand() - 0.5f) * width;
    v->gainL = std::sqrt(1.0f - pan);
    v->gainR = std::sqrt(pan);
    v->phase = 0.0;
    v->t = 0;
}

void Drops::process(const float* const*, int, float* const* out, int numOut,
                    int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* oL = out[0];
    float* oR = numOut > 1 ? out[1] : nullptr;
    for (int c = 2; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double density = std::clamp(params.get("Density", 4.0), 0.0, 20.0);
    const double size = std::clamp(params.get("Size", 0.5), 0.0, 1.0);
    const double spread = std::clamp(params.get("Spread", 0.5), 0.0, 1.0);
    const double chirp = std::clamp(params.get("Chirp", 0.6), 0.0, 1.0);
    const float pool = (float) std::clamp(params.get("Pool", 0.35), 0.0, 1.0);
    const float width = (float) std::clamp(params.get("Width", 0.7), 0.0, 1.0);
    const float level = (float) params.get("Level", 0.9);

    const bool raining = transport.playing() && density > 0.0;
    bool anyVoice = false;
    for (const auto& v : voices_) anyVoice = anyVoice || v.t >= 0;
    if (!raining && !anyVoice && std::abs(poolLpL_) < 1e-6f && std::abs(poolLpR_) < 1e-6f) {
        std::fill(oL, oL + numSamples, 0.0f);
        if (oR) std::fill(oR, oR + numSamples, 0.0f);
        return;
    }

    const float fb = 0.55f * pool;
    const float wet = 0.6f * pool;
    const float lpCoef = 0.35f;

    for (int i = 0; i < numSamples; ++i) {
        if (raining) {
            nextDrop_ -= 1.0;
            if (nextDrop_ <= 0.0) {
                trigger(size, spread, chirp, width);
                const double u = std::max(1e-6, (double) frand());
                nextDrop_ = -std::log(u) * sr / density;
            }
        }
        float l = 0.0f, r = 0.0f;
        for (auto& v : voices_) {
            if (v.t < 0) continue;
            const double ts = (double) v.t / sr;
            const double env = std::exp(-v.damp * ts);
            const double f = v.f0 * (1.0 + v.sigma * ts);
            v.phase += f / sr;
            if (v.phase >= 1.0) v.phase -= 1.0;
            double a = env;
            if (v.t < 24) a *= (double) v.t / 24.0;
            const float s = (float) (std::sin(v.phase * kTwoPi) * a) * v.amp;
            l += s * v.gainL;
            r += s * v.gainR;
            ++v.t;
            if (env < 1e-4) v.t = -1;
        }
        float& dl = poolBufL_[poolPosL_];
        float& dr = poolBufR_[poolPosR_];
        const float tailL = dl, tailR = dr;
        poolLpL_ += (tailL - poolLpL_) * lpCoef;
        poolLpR_ += (tailR - poolLpR_) * lpCoef;
        dl = l + r * 0.2f + poolLpL_ * fb;
        dr = r + l * 0.2f + poolLpR_ * fb;
        poolPosL_ = (poolPosL_ + 1) % poolBufL_.size();
        poolPosR_ = (poolPosR_ + 1) % poolBufR_.size();

        const float wl = (l + poolLpL_ * wet) * level;
        const float wr = (r + poolLpR_ * wet) * level;
        oL[i] = wl;
        if (oR) oR[i] = wr; else oL[i] = 0.5f * (wl + wr);
    }
}

}
