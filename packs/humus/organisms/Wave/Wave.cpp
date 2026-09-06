#include "Wave/Wave.h"

#include "hum/dsp/WaveWarp.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kFftOrder = 10;
static_assert((1 << kFftOrder) == kWaveTableLen, "table length is the FFT size");

}

void Wave::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    fft_ = std::make_unique<juce::dsp::FFT>(kFftOrder);
    tableReady_ = false;
    warpRamp_.assign((size_t) std::max(1, maxBlock), 0.0f);
    warpSm_ = 0.0;
    reset();
}

void Wave::reset() {
    for (auto& v : voices_) {
        v.note = -1;
        v.gate = false;
        v.env = 0.0;
        v.phase = 0.0;
        v.subPhase = 0.0;
    }
    next_ = 0;
    freePhase_ = 0.0;
    freeSubPhase_ = 0.0;
    freeEnv_ = 0.0;
    lp_.reset();
    lpHz_ = lpQ_ = -1.0;
}

void Wave::noteOn(int note, int vel) {
    int slot = -1;
    for (int i = 0; i < kVoices; ++i)
        if (voices_[(size_t) i].note == note) { slot = i; break; }
    if (slot < 0)
        for (int i = 0; i < kVoices; ++i)
            if (voices_[(size_t) i].note < 0) { slot = i; break; }
    if (slot < 0) { slot = next_; next_ = (next_ + 1) % kVoices; }

    Voice& v = voices_[(size_t) slot];
    v.note = note;
    v.vel = (float) vel / kMidiMaxF;
    v.gate = true;
    for (int u = 0; u < 6; ++u) v.uphase[u] = v.phase + (u + 1) * 0.137;
}

void Wave::noteOff(int note) {
    for (auto& v : voices_)
        if (v.note == note && v.gate) v.gate = false;
}

void Wave::rebuildTable(const std::string& text) {
    cachedText_ = text;
    const int n = decodeWaveFrames(text.c_str(), &frames_[0][0], kWaveTableLen, kMaxFrames);
    if (n > 0) frameCount_ = n;
    else if (!tableReady_) { waveTablePreset(0, frames_[0], kWaveTableLen); frameCount_ = 1; }
    waveTableAlignFrames(&frames_[0][0], frameCount_, kWaveTableLen);

    for (int f = 0; f < frameCount_; ++f) {
        std::fill(fwd_, fwd_ + 2 * kWaveTableLen, 0.0f);
        std::copy(frames_[f], frames_[f] + kWaveTableLen, fwd_);
        fft_->performRealOnlyForwardTransform(fwd_);
        std::copy(fwd_, fwd_ + 2 * kWaveTableLen, frameFft_[f]);
        for (int level = 0; level < kLevels; ++level) {
            const int cut = kWaveTableLen / 2 >> level;
            std::copy(fwd_, fwd_ + 2 * kWaveTableLen, lvl_);
            lvl_[0] = lvl_[1] = 0.0f;
            for (int k = 1; k < kWaveTableLen; ++k) {
                const int partial = k <= kWaveTableLen / 2 ? k : kWaveTableLen - k;
                if (partial > cut) { lvl_[2 * k] = 0.0f; lvl_[2 * k + 1] = 0.0f; }
            }
            fft_->performRealOnlyInverseTransform(lvl_);
            std::copy(lvl_, lvl_ + kWaveTableLen, frameMip_[f][level]);
        }
    }
    morphF0_ = morphF1_ = -1;
    tableReady_ = true;
}

void Wave::buildMorph(int f0, int f1, float fr) {
    const float* A = frameFft_[f0];
    const float* B = frameFft_[f1];
    for (int level = 0; level < kLevels; ++level) {
        const int cut = kWaveTableLen / 2 >> level;
        std::fill(lvl_, lvl_ + 2 * kWaveTableLen, 0.0f);
        for (int k = 1; k < kWaveTableLen; ++k) {
            const int partial = k <= kWaveTableLen / 2 ? k : kWaveTableLen - k;
            if (partial > cut) continue;
            const float m0 = std::hypot(A[2 * k], A[2 * k + 1]);
            const float m1 = std::hypot(B[2 * k], B[2 * k + 1]);
            const float mm = m0 + fr * (m1 - m0);
            const float ph = std::atan2(A[2 * k + 1], A[2 * k]);
            lvl_[2 * k] = mm * std::cos(ph);
            lvl_[2 * k + 1] = mm * std::sin(ph);
        }
        fft_->performRealOnlyInverseTransform(lvl_);
        std::copy(lvl_, lvl_ + kWaveTableLen, morphMip_[level]);
    }
    morphF0_ = f0; morphF1_ = f1; morphFr_ = fr;
}

void Wave::process(const float* const*, int, float* const* out, int numOut,
                   int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];
    std::fill(L, L + numSamples, 0.0f);
    if (R != L) std::fill(R, R + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;

    {
        const auto* tp = params.byName("Table");
        if (!tableReady_ || (tp != nullptr && tp->text != cachedText_))
            rebuildTable(tp != nullptr ? tp->text : std::string());
    }

    {
        std::lock_guard<std::mutex> g(liveLock_);
        for (int i = 0; i < liveCount_; ++i)
            if (stagedCount_ < (int) staged_.size()) staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
        liveCount_ = 0;
    }
    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(size_t) i];
        const int st = e.data[0] & 0xF0;
        if (st == 0x90 && e.data[2] > 0) noteOn(e.data[1], e.data[2]);
        else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) noteOff(e.data[1]);
    }
    stagedCount_ = 0;

    const bool poly = params.get("Mode", 1.0) >= 0.5;
    const double note = std::clamp(params.get("Note", 48.0), 0.0, kMidiMaxD);
    const double attack = std::max(1.0, params.get("Attack", 5.0));
    const double release = std::max(20.0, params.get("Release", 400.0));
    const float sub = (float) std::clamp(params.get("Sub", 0.0), 0.0, 1.0);
    const double cutoff = params.get("Cutoff", 14000.0);
    const double resoQ = 0.707 * std::pow(10.0, std::clamp(params.get("Reso", 0.0), 0.0, 1.0) * 1.05);
    const float drive = (float) std::clamp(params.get("Drive", 0.0), 0.0, 1.0);
    const float level = (float) params.get("Level", 0.8);
    const int warpMode = (int) params.get("WarpMode", 0.0);
    const float warpTarget = (float) std::clamp(params.get("Warp", 0.0), 0.0, 1.0);
    if ((int) warpRamp_.size() < numSamples) warpRamp_.assign((size_t) numSamples, 0.0f);
    const double warpCoef = 1.0 - std::exp(-1.0 / (0.006 * sr));
    for (int i = 0; i < numSamples; ++i) {
        warpSm_ += warpCoef * (warpTarget - warpSm_);
        warpRamp_[(size_t) i] = (float) warpSm_;
    }
    const double basePos = std::clamp(params.get("Position", 0.0), 0.0, 1.0);
    const float posMod = (float) std::clamp(params.get("PositionMod", 0.0), 0.0, 1.0);
    const int uni = std::clamp((int) params.get("Unison", 1.0), 1, 7);
    const double detune = std::clamp(params.get("Detune", 0.0), 0.0, 1.0);
    const float width = (float) std::clamp(params.get("Width", 0.0), 0.0, 1.0);
    const int frames = frameCount_;
    auto pickFrames = [&](double env, int& f0, int& f1, float& fmix) {
        const double fp = std::clamp(basePos + env * posMod, 0.0, 1.0) * (frames - 1);
        f0 = std::clamp((int) fp, 0, frames - 1);
        f1 = std::min(f0 + 1, frames - 1);
        fmix = (float) (fp - f0);
    };
    const bool spectral = params.get("Spectral", 0.0) >= 0.5 && frameCount_ > 1;
    if (spectral) {
        const double fp = basePos * (frameCount_ - 1);
        const int sf0 = std::clamp((int) fp, 0, frameCount_ - 1);
        const int sf1 = std::min(sf0 + 1, frameCount_ - 1);
        const float sfr = (float) (fp - sf0);
        if (sf0 != morphF0_ || sf1 != morphF1_ || std::abs(sfr - morphFr_) > 0.003f)
            buildMorph(sf0, sf1, sfr);
    }

    const double atk = 1.0 - std::exp(-1.0 / (attack * 0.001 * sr));
    const double rel = 1.0 - std::exp(-1.0 / (release * 0.001 * sr));
    const double freeSm = 1.0 - std::exp(-1.0 / (0.010 * sr));

    const auto& tuning = transport.tuning();

    if (!poly || freeEnv_ >= 1.0e-4) {
        const double hz = tuning.hz(note);
        const double dt = std::clamp(hz / sr, 0.0, 0.49);
        const int level = levelFor(hz, sr);
        int f0, f1; float fmix; pickFrames(freeEnv_, f0, f1, fmix);
        const float* t0 = spectral ? morphMip_[level] : frameMip_[f0][level];
        const float* t1 = frameMip_[f1][level];
        if (spectral) fmix = 0.0f;
        const double target = poly ? 0.0 : 1.0;
        for (int i = 0; i < numSamples; ++i) {
            freeEnv_ += freeSm * (target - freeEnv_);
            float w = warpRead(t0, kWaveTableLen, freePhase_, warpMode, warpRamp_[(size_t) i]);
            if (fmix > 0.0f)
                w += fmix * (warpRead(t1, kWaveTableLen, freePhase_, warpMode, warpRamp_[(size_t) i]) - w);
            if (sub > 0.0f) {
                w += sub * (float) std::sin(kTwoPi * freeSubPhase_);
                freeSubPhase_ += dt * 0.5;
                if (freeSubPhase_ >= 1.0) freeSubPhase_ -= 1.0;
            }
            const float s = w * (float) freeEnv_;
            freePhase_ += dt;
            if (freePhase_ >= 1.0) freePhase_ -= 1.0;
            L[i] += s;
            if (R != L) R[i] += s;
        }
    } else {
        freeEnv_ = 0.0;
    }

    for (auto& v : voices_) {
        if (v.note < 0) continue;
        v.hz = tuning.hz((double) v.note);
        const double dt = std::clamp(v.hz / sr, 0.0, 0.49);
        const int level = levelFor(v.hz, sr);
        int f0, f1; float fmix; pickFrames(v.env, f0, f1, fmix);
        const float* t0 = spectral ? morphMip_[level] : frameMip_[f0][level];
        const float* t1 = frameMip_[f1][level];
        if (spectral) fmix = 0.0f;
        const double gain = poly ? (double) v.vel : 0.0;
        double urate[6];
        const int extra = uni - 1;
        for (int u = 0; u < extra; ++u) {
            const double off = extra > 1 ? (double) u / (extra - 1) - 0.5 : 0.0;
            urate[u] = std::pow(2.0, off * detune * 100.0 / 1200.0);
        }
        const float unorm = 1.0f / std::sqrt((float) uni);
        for (int i = 0; i < numSamples; ++i) {
            v.env += (v.gate ? atk * (1.0 - v.env) : -rel * v.env);
            auto read = [&](double ph) {
                float a = warpRead(t0, kWaveTableLen, ph, warpMode, warpRamp_[(size_t) i]);
                if (fmix > 0.0f)
                    a += fmix * (warpRead(t1, kWaveTableLen, ph, warpMode, warpRamp_[(size_t) i]) - a);
                return a;
            };
            const float a0 = read(v.phase);
            float wL = a0, wR = a0;
            for (int u = 0; u < extra; ++u) {
                const float a = read(v.uphase[u]);
                const double poff = extra > 1 ? (double) u / (extra - 1) - 0.5 : 0.0;
                const float pan = (float) (poff * 2.0 * width);
                wL += (1.0f - std::max(0.0f, pan)) * a;
                wR += (1.0f - std::max(0.0f, -pan)) * a;
                v.uphase[u] += dt * urate[u];
                if (v.uphase[u] >= 1.0) v.uphase[u] -= 1.0;
            }
            wL *= unorm; wR *= unorm;
            if (sub > 0.0f) {
                const float sv = sub * (float) std::sin(kTwoPi * v.subPhase);
                wL += sv; wR += sv;
                v.subPhase += dt * 0.5;
                if (v.subPhase >= 1.0) v.subPhase -= 1.0;
            }
            const float eg = (float) (v.env * gain);
            v.phase += dt;
            if (v.phase >= 1.0) v.phase -= 1.0;
            L[i] += wL * eg;
            if (R != L) R[i] += wR * eg;
        }
        if (!v.gate && v.env < 1.0e-4) { v.note = -1; v.env = 0.0; }
    }

    if (cutoff < 13900.0) {
        if (cutoff != lpHz_ || resoQ != lpQ_) {
            const double fc = std::min(cutoff, sr * 0.45);
            lp_.setLowpass(sr, fc, resoQ);
            lpR_.setLowpass(sr, fc, resoQ);
            lpHz_ = cutoff; lpQ_ = resoQ;
        }
        for (int i = 0; i < numSamples; ++i) L[i] = lp_.process(L[i]);
        if (R != L) for (int i = 0; i < numSamples; ++i) R[i] = lpR_.process(R[i]);
    }
    if (drive > 0.0f) {
        const float g = 1.0f + 5.0f * drive;
        const float wet = std::min(1.0f, drive * 12.0f);
        auto sat = [&](float x) { return x + wet * (std::tanh(x * g) - x); };
        for (int i = 0; i < numSamples; ++i) L[i] = sat(L[i]);
        if (R != L) for (int i = 0; i < numSamples; ++i) R[i] = sat(R[i]);
    }

    for (int i = 0; i < numSamples; ++i) {
        L[i] *= level;
        if (R != L) R[i] *= level;
    }
}

}
