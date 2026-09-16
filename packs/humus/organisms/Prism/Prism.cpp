// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Prism/Prism.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kOrder = 11;
constexpr double kMinF0 = 30.0, kMaxF0 = 1400.0;
constexpr double kF0SmMs = 15.0;
constexpr double kGateSmMs = 20.0;
constexpr double kAmpSmMs = 5.0;

constexpr const char* kHarmName[Prism::kHarms] = {
    "Harm_1", "Harm_2", "Harm_3", "Harm_4", "Harm_5", "Harm_6", "Harm_7", "Harm_8",
    "Harm_9", "Harm_10", "Harm_11", "Harm_12", "Harm_13", "Harm_14", "Harm_15", "Harm_16",
};
}

void Prism::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    const int N = 1 << kOrder;
    tracker_.prepare(sampleRate);
    tracker_.setHzRange(kMinF0, kMaxF0);
    chans_.clear();
    chans_.resize(2);
    for (auto& ch : chans_) {
        ch.stft.prepare(kOrder, maxBlock);
        ch.dry.assign((size_t) N, 0.0f);
        ch.dryPos = 0;
        ch.stft.setSpectrumFn([this](float* reim, int fftSize) { onFrame(reim, fftSize); });
    }
    wet_.assign((size_t) std::max(1, maxBlock), 0.0f);
    mono_.assign((size_t) std::max(1, maxBlock), 0.0f);
    bank_.assign((size_t) std::max(1, maxBlock), 0.0f);
    silence_.assign((size_t) std::max(1, maxBlock), 0.0f);
    bankDelay_.assign((size_t) N, 0.0f);
    bankPos_ = 0;
    reset();
}

void Prism::reset() {
    tracker_.reset();
    for (auto& ch : chans_) {
        ch.stft.reset();
        std::fill(ch.dry.begin(), ch.dry.end(), 0.0f);
        ch.dryPos = 0;
    }
    std::fill(bankDelay_.begin(), bankDelay_.end(), 0.0f);
    bankPos_ = 0;
    for (auto& h : harms_) h = Harm{};
    f0_ = f0Target_ = 0.0;
    gate_ = 0.0;
    pubHz_.store(0.0f, std::memory_order_relaxed);
    pubClarity_.store(0.0f, std::memory_order_relaxed);
    pubLevel_.store(0.0f, std::memory_order_relaxed);
    pubNote_.store(-1, std::memory_order_relaxed);
}

void Prism::onFrame(float* reim, int fftSize) {
    const int bins = fftSize / 2 + 1;
    const double f0 = (double) frameF0_;
    if (f0 < kMinF0) return;
    const double binHz = sampleRate_ / (double) fftSize;
    const double width = std::max(2.0 * binHz, 0.25 * f0);
    for (int k = 0; k < bins; ++k) {
        const double freq = k * binHz;
        const int h = std::max(1, (int) std::lround(freq / f0));
        const bool harmonic = std::abs(freq - h * f0) < width;
        const float g = harmonic ? (h <= kHarms ? frameGains_[h - 1] : 1.0f)
                                 : frameResidual_;
        reim[2 * k] *= g;
        reim[2 * k + 1] *= g;
    }
}

void Prism::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport& transport) {
    const bool filter = params.get("Mode", 0.0) >= 0.5;
    const double disp = std::clamp(params.get("Dispersion", 0.0), -5.0, 15.0) / 100.0;
    const float residual = (float) std::clamp(params.get("Residual", 1.0), 0.0, 2.0);
    const float mix = (float) std::clamp(params.get("Mix", 1.0), 0.0, 1.0);
    const float level = (float) params.get("Level", 0.8);
    float gains[kHarms];
    for (int k = 0; k < kHarms; ++k)
        gains[k] = (float) std::clamp(params.get(kHarmName[k], 1.0), 0.0, 2.0);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const int n = std::min(numSamples, (int) wet_.size());

    for (int i = 0; i < n; ++i) {
        float s = 0.0f;
        if (numIn > 0 && in[0]) s += in[0][i];
        if (numIn > 1 && in[1]) s = 0.5f * (s + in[1][i]);
        mono_[(size_t) i] = s;
    }

    bool voiced = gate_ > 0.5;
    if (tracker_.push(mono_.data(), n) > 0) {
        const double hz = tracker_.pitchHz();
        voiced = hz >= kMinF0 && hz <= kMaxF0 && tracker_.clarity() > 0.4;
        if (voiced) {
            f0Target_ = hz;
            if (f0_ <= 0.0) f0_ = hz;
        }
    }
    const double gateTarget = voiced ? 1.0 : 0.0;
    const double f0Sm = 1.0 - std::exp(-1.0 / (kF0SmMs * 0.001 * sr));
    const double gateSm = 1.0 - std::exp(-1.0 / (kGateSmMs * 0.001 * sr));

    if (!filter) {
        const double ampSm = 1.0 - std::exp(-1.0 / (kAmpSmMs * 0.001 * sr));
        const double fc = f0_ > 0.0 ? std::min(25.0, 0.12 * f0_) : 25.0;
        const double lp = 1.0 - std::exp(-kTwoPi * fc / sr);
        for (int i = 0; i < n; ++i) {
            f0_ += f0Sm * (f0Target_ - f0_);
            gate_ += gateSm * (gateTarget - gate_);
            const double x = mono_[(size_t) i];
            double sum = 0.0;
            double e1 = 0.0;
            for (int k = 0; k < kHarms; ++k) {
                Harm& h = harms_[k];
                const double fA = (k + 1) * f0_;
                double target = 0.0;
                if (f0_ >= kMinF0 && fA < 0.45 * sr) {
                    h.aPhase += fA / sr;
                    if (h.aPhase >= 1.0) h.aPhase -= 1.0;
                    const double ang = kTwoPi * h.aPhase;
                    const double c = std::cos(ang), s = -std::sin(ang);
                    h.zr1 += lp * (x * c - h.zr1);
                    h.zi1 += lp * (x * s - h.zi1);
                    h.zr2 += lp * (h.zr1 - h.zr2);
                    h.zi2 += lp * (h.zi1 - h.zi2);
                    const double e = 2.0 * std::sqrt(h.zr2 * h.zr2 + h.zi2 * h.zi2);
                    if (k == 0) e1 = e;
                    const double g = (double) gains[k];
                    target = gate_ * (std::min(g, 1.0) * e + std::max(g - 1.0, 0.0) * e1);
                }
                h.amp += ampSm * (target - h.amp);
                const double fS = fA * (1.0 + disp * (double) k);
                if (fS < 0.45 * sr && f0_ > 0.0) {
                    h.sPhase += fS / sr;
                    if (h.sPhase >= 1.0) h.sPhase -= 1.0;
                    sum += h.amp * std::sin(kTwoPi * h.sPhase);
                }
            }
            bank_[(size_t) i] = (float) sum;
        }
        const int N = (int) bankDelay_.size();
        for (int i = 0; i < n; ++i) {
            const float delayed = bankDelay_[(size_t) bankPos_];
            bankDelay_[(size_t) bankPos_] = bank_[(size_t) i];
            bankPos_ = (bankPos_ + 1) % N;
            wet_[(size_t) i] = delayed;
        }
    } else {
        const double bn = (double) n;
        f0_ += (1.0 - std::pow(1.0 - f0Sm, bn)) * (f0Target_ - f0_);
        gate_ += (1.0 - std::pow(1.0 - gateSm, bn)) * (gateTarget - gate_);
        for (auto& h : harms_) h.amp = 0.0;
        frameF0_ = gate_ > 0.5 ? (float) f0_ : 0.0f;
        for (int k = 0; k < kHarms; ++k) frameGains_[k] = gains[k];
        frameResidual_ = residual;
    }

    pubHz_.store(gate_ > 0.5 ? (float) f0_ : 0.0f, std::memory_order_relaxed);
    pubClarity_.store((float) tracker_.clarity(), std::memory_order_relaxed);
    pubLevel_.store((float) tracker_.level(), std::memory_order_relaxed);
    pubNote_.store(gate_ > 0.5 && f0_ > 0.0
                       ? (int) std::lround(transport.tuning().midiNote(f0_))
                       : -1,
                   std::memory_order_relaxed);

    for (int c = 0; c < numOut; ++c) {
        float* o = out[c];
        if (c >= (int) chans_.size()) { std::fill(o, o + numSamples, 0.0f); continue; }
        auto& ch = chans_[(size_t) c];
        const float* inp = (c < numIn && in[c]) ? in[c] : silence_.data();

        if (filter) ch.stft.process(inp, wet_.data(), n);

        const int N = (int) ch.dry.size();
        for (int i = 0; i < n; ++i) {
            const float dry = ch.dry[(size_t) ch.dryPos];
            ch.dry[(size_t) ch.dryPos] = inp[i];
            ch.dryPos = (ch.dryPos + 1) % N;
            const float w = wet_[(size_t) i] * level;
            o[i] = (1.0f - mix) * dry + mix * w;
        }
        for (int i = n; i < numSamples; ++i) o[i] = 0.0f;
    }
}

}
