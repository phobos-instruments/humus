#include "SpectralFreeze/SpectralFreeze.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kOrder = 10;
}

void SpectralFreeze::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    const int N = 1 << kOrder;
    const int bins = N / 2 + 1;
    frameRate_ = sampleRate / (double) (N / 4);

    chans_.clear();
    chans_.resize((size_t) std::max(1, ch_));
    for (size_t c = 0; c < chans_.size(); ++c) {
        auto& ch = chans_[c];
        ch.stft.prepare(kOrder, maxBlock);
        ch.heldMag.assign((size_t) bins, 0.0f);
        ch.prevPhase.assign((size_t) bins, 0.0f);
        ch.advance.assign((size_t) bins, 0.0f);
        ch.outPhase.assign((size_t) bins, 0.0f);
        ch.inMag.assign((size_t) bins, 0.0f);
        ch.blur.assign((size_t) bins, 0.0f);
        ch.dry.assign((size_t) N, 0.0f);
        ch.dryPos = 0;
        ch.fSmoothed = 0.0f;
        Chan* cp = &ch;
        ch.stft.setSpectrumFn([this, cp](float* reim, int fftSize) { onFrame(*cp, reim, fftSize); });
    }
    wet_.assign((size_t) std::max(1, maxBlock), 0.0f);
    silence_.assign((size_t) std::max(1, maxBlock), 0.0f);
}

void SpectralFreeze::reset() {
    for (auto& ch : chans_) {
        ch.stft.reset();
        std::fill(ch.heldMag.begin(), ch.heldMag.end(), 0.0f);
        std::fill(ch.prevPhase.begin(), ch.prevPhase.end(), 0.0f);
        std::fill(ch.advance.begin(), ch.advance.end(), 0.0f);
        std::fill(ch.outPhase.begin(), ch.outPhase.end(), 0.0f);
        std::fill(ch.dry.begin(), ch.dry.end(), 0.0f);
        ch.dryPos = 0;
        ch.fSmoothed = 0.0f;
    }
}

void SpectralFreeze::process(const float* const* in, int numIn, float* const* out, int numOut,
                             int numSamples, const Transport&) {
    const bool freeze = params.get("Freeze", 0.0) >= 0.5;
    const float mix   = (float) std::clamp(params.get("Mix", 1.0), 0.0, 1.0);
    smear_     = (float) std::clamp(params.get("Smear", 0.0), 0.0, 1.0);
    diffusion_ = (float) std::clamp(params.get("Diffusion", 0.0), 0.0, 1.0);
    shimmer_   = (float) std::clamp(params.get("Shimmer", 0.0), 0.0, 1.0);
    frozen_    = freeze;
    fTarget_   = freeze ? 1.0f : smear_;

    const double tau = 0.001 * std::pow(10000.0, (double) smear_);
    smearCoeff_ = (float) std::exp(-1.0 / (tau * frameRate_));
    fCoeff_     = (float) std::exp(-1.0 / (0.03 * frameRate_));

    const int n = std::min(numSamples, (int) wet_.size());
    for (int c = 0; c < numOut; ++c) {
        float* o = out[c];
        if (c >= (int) chans_.size()) { std::fill(o, o + numSamples, 0.0f); continue; }
        auto& ch = chans_[(size_t) c];
        const float* inp = (c < numIn && in[c]) ? in[c] : silence_.data();

        ch.stft.process(inp, wet_.data(), n);

        const int N = (int) ch.dry.size();
        for (int i = 0; i < n; ++i) {
            const float dry = ch.dry[(size_t) ch.dryPos];
            ch.dry[(size_t) ch.dryPos] = inp[i];
            ch.dryPos = (ch.dryPos + 1) % N;
            o[i] = (1.0f - mix) * dry + mix * wet_[(size_t) i];
        }
        for (int i = n; i < numSamples; ++i) o[i] = 0.0f;
    }
}

void SpectralFreeze::onFrame(Chan& c, float* reim, int fftSize) {
    const int bins = fftSize / 2 + 1;
    c.fSmoothed += (fTarget_ - c.fSmoothed) * (1.0f - fCoeff_);
    const float f = c.fSmoothed;

    for (int k = 0; k < bins; ++k) {
        const float re = reim[2 * k], im = reim[2 * k + 1];
        const float mag = std::sqrt(re * re + im * im);
        const float ph  = std::atan2(im, re);
        c.inMag[(size_t) k] = mag;
        if (!frozen_) {
            float d = ph - c.prevPhase[(size_t) k];
            d -= kTwoPiF * std::round(d / kTwoPiF);
            c.advance[(size_t) k] = d;
            c.heldMag[(size_t) k] += (1.0f - smearCoeff_) * (mag - c.heldMag[(size_t) k]);
        }
        c.prevPhase[(size_t) k] = ph;

        float adv = c.advance[(size_t) k];
        if (diffusion_ > 0.0f) adv += diffusion_ * 0.15f * (nextRand() - 0.5f);
        float p = c.outPhase[(size_t) k] + adv;
        if (p > kTwoPiF) p -= kTwoPiF; else if (p < -kTwoPiF) p += kTwoPiF;
        c.outPhase[(size_t) k] = p;
    }

    const float d = diffusion_ * 0.85f;
    if (d > 1.0e-4f) {
        float a = 0.0f;
        for (int k = 0; k < bins; ++k) { a = d * a + (1.0f - d) * c.heldMag[(size_t) k]; c.blur[(size_t) k] = a; }
        float b = 0.0f;
        for (int k = bins - 1; k >= 0; --k) {
            b = d * b + (1.0f - d) * c.heldMag[(size_t) k];
            c.blur[(size_t) k] = 0.5f * (c.blur[(size_t) k] + b);
        }
    } else {
        std::copy(c.heldMag.begin(), c.heldMag.begin() + bins, c.blur.begin());
    }

    for (int k = 0; k < bins; ++k) {
        float mag = c.blur[(size_t) k];
        if (shimmer_ > 0.0f) mag += shimmer_ * c.blur[(size_t) (k / 2)];
        const float synRe = mag * std::cos(c.outPhase[(size_t) k]);
        const float synIm = mag * std::sin(c.outPhase[(size_t) k]);

        const float im0 = c.inMag[(size_t) k];
        const float scale = im0 > 1.0e-9f ? mag / im0 : 0.0f;
        const float pasRe = reim[2 * k]     * scale;
        const float pasIm = reim[2 * k + 1] * scale;

        reim[2 * k]     = pasRe + f * (synRe - pasRe);
        reim[2 * k + 1] = pasIm + f * (synIm - pasIm);
    }
}

}
