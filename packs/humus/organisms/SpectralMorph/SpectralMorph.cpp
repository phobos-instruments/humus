#include "SpectralMorph/SpectralMorph.h"

#include <algorithm>
#include <cmath>

namespace hum {

namespace {
constexpr int kOrder = 10;
}

void SpectralMorph::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    const int N = 1 << kOrder;
    bins_ = N / 2 + 1;
    entryLen_ = 2 * bins_;
    const int hop = N / 4;
    fCap_ = maxBlock / hop + 4;

    chans_.clear();
    chans_.resize((size_t) std::max(1, ch_));
    for (auto& c : chans_) {
        c.a.prepare(kOrder, maxBlock);
        c.b.prepare(kOrder, maxBlock);
        c.aFifo.assign((size_t) (fCap_ * entryLen_), 0.0f);
        c.fHead = c.fTail = c.fCount = 0;
        c.dry.assign((size_t) N, 0.0f);
        c.dryPos = 0;
        Chan* cp = &c;
        c.a.setSpectrumFn([this, cp](float* reim, int) { pushA(*cp, reim); });
        c.b.setSpectrumFn([this, cp](float* reim, int) { blendB(*cp, reim); });
    }
    discard_.assign((size_t) std::max(1, maxBlock), 0.0f);
    silence_.assign((size_t) std::max(1, maxBlock), 0.0f);
}

void SpectralMorph::reset() {
    for (auto& c : chans_) {
        c.a.reset(); c.b.reset();
        std::fill(c.aFifo.begin(), c.aFifo.end(), 0.0f);
        c.fHead = c.fTail = c.fCount = 0;
        std::fill(c.dry.begin(), c.dry.end(), 0.0f);
        c.dryPos = 0;
    }
}

void SpectralMorph::pushA(Chan& c, const float* reim) {
    float* slot = c.aFifo.data() + (size_t) c.fTail * entryLen_;
    std::copy(reim, reim + entryLen_, slot);
    c.fTail = (c.fTail + 1) % fCap_;
    if (c.fCount < fCap_) ++c.fCount; else c.fHead = (c.fHead + 1) % fCap_;
}

void SpectralMorph::blendB(Chan& c, float* reim) {
    const float* a = c.fCount > 0 ? c.aFifo.data() + (size_t) c.fHead * entryLen_ : nullptr;
    if (a) { c.fHead = (c.fHead + 1) % fCap_; --c.fCount; }
    const float m = morph_;
    for (int k = 0; k < bins_; ++k) {
        const float aRe = a ? a[2 * k] : 0.0f, aIm = a ? a[2 * k + 1] : 0.0f;
        const float bRe = reim[2 * k],        bIm = reim[2 * k + 1];
        const float magA = std::sqrt(aRe * aRe + aIm * aIm);
        const float magB = std::sqrt(bRe * bRe + bIm * bIm);
        const float mag = (1.0f - m) * magA + m * magB;
        const bool aWins = (1.0f - m) * magA >= m * magB;
        const float ph = aWins ? std::atan2(aIm, aRe) : std::atan2(bIm, bRe);
        reim[2 * k]     = mag * std::cos(ph);
        reim[2 * k + 1] = mag * std::sin(ph);
    }
}

void SpectralMorph::process(const float* const* in, int numIn, float* const* out, int numOut,
                            int numSamples, const Transport&) {
    morph_ = (float) std::clamp(params.get("Morph", 0.0), 0.0, 1.0);

    const int n = std::min(numSamples, (int) discard_.size());
    for (int c = 0; c < numOut && c < (int) chans_.size(); ++c) {
        auto& ch = chans_[(size_t) c];
        const float* inA = (c < numIn && in[c]) ? in[c] : silence_.data();
        const float* inB = (c + ch_ < numIn && in[c + ch_]) ? in[c + ch_] : silence_.data();
        float* o = out[c];

        const int N = (int) ch.dry.size();
        ch.a.process(inA, discard_.data(), n);
        ch.b.process(inB, o, n);
        for (int i = 0; i < n; ++i) {
            ch.dry[(size_t) ch.dryPos] = inA[i];
            ch.dryPos = (ch.dryPos + 1) % N;
        }
        for (int i = n; i < numSamples; ++i) o[i] = 0.0f;
    }
    for (int c = (int) chans_.size(); c < numOut; ++c)
        std::fill(out[c], out[c] + numSamples, 0.0f);
}

}
