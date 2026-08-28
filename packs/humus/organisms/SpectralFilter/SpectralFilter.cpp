#include "SpectralFilter/SpectralFilter.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kOrder = 10;
constexpr float kMaxTiltDb = 18.0f;
constexpr float kEnvBlur = 0.9f;
}

void SpectralFilter::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    const int N = 1 << kOrder;
    const int bins = N / 2 + 1;
    chans_.clear();
    chans_.resize((size_t) std::max(1, ch_));
    for (auto& ch : chans_) {
        ch.stft.prepare(kOrder, maxBlock);
        ch.mag.assign((size_t) bins, 0.0f);
        ch.env.assign((size_t) bins, 0.0f);
        ch.dry.assign((size_t) N, 0.0f);
        ch.dryPos = 0;
        Chan* cp = &ch;
        ch.stft.setSpectrumFn([this, cp](float* reim, int fftSize) { onFrame(*cp, reim, fftSize); });
    }
    wet_.assign((size_t) std::max(1, maxBlock), 0.0f);
    silence_.assign((size_t) std::max(1, maxBlock), 0.0f);
}

void SpectralFilter::reset() {
    for (auto& ch : chans_) {
        ch.stft.reset();
        std::fill(ch.dry.begin(), ch.dry.end(), 0.0f);
        ch.dryPos = 0;
    }
}

void SpectralFilter::onFrame(Chan& c, float* reim, int fftSize) {
    const int bins = fftSize / 2 + 1;
    for (int k = 0; k < bins; ++k) {
        const float re = reim[2 * k], im = reim[2 * k + 1];
        c.mag[(size_t) k] = std::sqrt(re * re + im * im);
    }
    {
        const float d = kEnvBlur;
        float a = 0.0f;
        for (int k = 0; k < bins; ++k) { a = d * a + (1.0f - d) * c.mag[(size_t) k]; c.env[(size_t) k] = a; }
        float b = 0.0f;
        for (int k = bins - 1; k >= 0; --k) {
            b = d * b + (1.0f - d) * c.mag[(size_t) k];
            c.env[(size_t) k] = 0.5f * (c.env[(size_t) k] + b);
        }
    }
    const float exponent = 1.0f + contrast_;
    const float invBins = bins > 1 ? 1.0f / (float) (bins - 1) : 0.0f;
    for (int k = 0; k < bins; ++k) {
        const float m0 = c.mag[(size_t) k];
        if (m0 <= 1.0e-9f) { reim[2 * k] = 0.0f; reim[2 * k + 1] = 0.0f; continue; }
        const float tiltDb = tilt_ * kMaxTiltDb * (2.0f * (float) k * invBins - 1.0f);
        const float tiltLin = (float) dbToLin(tiltDb);
        const float env = c.env[(size_t) k] > 1.0e-9f ? c.env[(size_t) k] : 1.0e-9f;
        const float ratio = m0 / env;
        const float newMag = tiltLin * env * std::pow(ratio, exponent);
        const float scale = newMag / m0;
        reim[2 * k]     *= scale;
        reim[2 * k + 1] *= scale;
    }
}

void SpectralFilter::process(const float* const* in, int numIn, float* const* out, int numOut,
                             int numSamples, const Transport&) {
    const float mix = (float) std::clamp(params.get("Mix", 1.0), 0.0, 1.0);
    tilt_     = (float) std::clamp(params.get("Tilt", 0.0), -1.0, 1.0);
    contrast_ = (float) std::clamp(params.get("Contrast", 0.0), -1.0, 1.0);

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

}
