// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Wave/Wave.h"

#include "hum/dsp/WaveWarp.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kFftOrder = 10;
static_assert((1 << kFftOrder) == kWaveTableLen, "table length is the FFT size");

}

void Wave::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    fft_ = std::make_unique<juce::dsp::FFT>(kFftOrder);
    syncTable();
    warpRamp_.assign((size_t) std::max(1, maxBlock), 0.0f);
    warpSm_ = 0.0;
    driveOsL_.prepare(4);
    driveOsR_.prepare(4);
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
    bend_.reset();
    bendRatio_ = 1.0;
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

std::unique_ptr<Wave::Table> Wave::buildTable(const std::string& text, bool presetWhenEmpty) {
    auto table = std::make_unique<Table>();
    const int n = decodeWaveFrames(text.c_str(), &table->frames[0][0], kWaveTableLen, kMaxFrames);
    if (n > 0) table->frameCount = n;
    else if (presetWhenEmpty) { waveTablePreset(0, table->frames[0], kWaveTableLen); table->frameCount = 1; }
    else return nullptr;
    waveTableAlignFrames(&table->frames[0][0], table->frameCount, kWaveTableLen);

    juce::dsp::FFT fft(kFftOrder);
    std::vector<float> fwd(2 * kWaveTableLen, 0.0f), lvl(2 * kWaveTableLen, 0.0f);
    for (int f = 0; f < table->frameCount; ++f) {
        std::fill(fwd.begin(), fwd.end(), 0.0f);
        std::copy(table->frames[f], table->frames[f] + kWaveTableLen, fwd.begin());
        fft.performRealOnlyForwardTransform(fwd.data());
        std::copy(fwd.begin(), fwd.end(), table->fft[f]);
        for (int level = 0; level < kLevels; ++level) {
            const int cut = kWaveTableLen / 2 >> level;
            std::copy(fwd.begin(), fwd.end(), lvl.begin());
            lvl[0] = lvl[1] = 0.0f;
            for (int k = 1; k < kWaveTableLen; ++k) {
                const int partial = k <= kWaveTableLen / 2 ? k : kWaveTableLen - k;
                if (partial > cut) { lvl[(size_t) (2 * k)] = 0.0f; lvl[(size_t) (2 * k + 1)] = 0.0f; }
            }
            fft.performRealOnlyInverseTransform(lvl.data());
            std::copy(lvl.begin(), lvl.begin() + kWaveTableLen, table->mip[f][level]);
        }
    }
    return table;
}

void Wave::syncTable() {
    const std::string text = params.getText("Table");
    if (table_ && text == appliedText_) return;
    if (auto fresh = buildTable(text, table_ == nullptr)) {
        table_ = std::move(fresh);
        morphF0_ = morphF1_ = -1;
    }
    appliedText_ = text;
}

void Wave::loadFrom(const OrganismState& state) {
    Organism::loadFrom(state);
    syncTable();
}

void Wave::onTextChanged(const std::string& param, const std::string& text) {
    if (param != "Table") return;
    appliedText_ = text;
    if (auto fresh = buildTable(text, false)) pendingTable_.publish(std::move(fresh));
}

void Wave::buildMorph(int f0, int f1, float fr) {
    const float* A = table_->fft[f0];
    const float* B = table_->fft[f1];
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

    if (pendingTable_.adopt(table_)) morphF0_ = morphF1_ = -1;
    if (!table_) return;

    if (std::unique_lock<std::mutex> g(liveLock_, std::try_to_lock); g.owns_lock()) {
        for (int i = 0; i < liveCount_; ++i)
            if (stagedCount_ < (int) staged_.size()) staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
        liveCount_ = 0;
    }
    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(size_t) i];
        if (bend_.apply(e)) continue;
        const int st = e.data[0] & 0xF0;
        if (st == 0x90 && e.data[2] > 0) noteOn(e.data[1], e.data[2]);
        else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) noteOff(e.data[1]);
    }
    stagedCount_ = 0;
    const double bendFrom = bendRatio_;
    bendRatio_ = bend_.ratio(bendRangeOf(params));
    const double bendStep = (bendRatio_ - bendFrom) / (double) std::max(1, numSamples);

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
    if (!posPrimed_) { posSm_ = basePos; posPrimed_ = true; }
    if ((int) posRamp_.size() < numSamples) posRamp_.assign((size_t) numSamples, 0.0f);
    const double posCoef = 1.0 - std::exp(-1.0 / (0.008 * sr));
    for (int i = 0; i < numSamples; ++i) {
        posSm_ += posCoef * (basePos - posSm_);
        posRamp_[(size_t) i] = (float) posSm_;
    }
    const int uni = std::clamp((int) params.get("Unison", 1.0), 1, 7);
    const double detune = std::clamp(params.get("Detune", 0.0), 0.0, 1.0);
    const float width = (float) std::clamp(params.get("Width", 0.0), 0.0, 1.0);
    const int frames = table_->frameCount;
    auto pickFrames = [&](double where, double env, int& f0, int& f1, float& fmix) {
        const double fp = std::clamp(where + env * posMod, 0.0, 1.0) * (frames - 1);
        f0 = std::clamp((int) fp, 0, frames - 1);
        f1 = std::min(f0 + 1, frames - 1);
        fmix = (float) (fp - f0);
    };
    const bool spectral = params.get("Spectral", 0.0) >= 0.5 && table_->frameCount > 1;
    if (spectral) {
        const double fp = posSm_ * (table_->frameCount - 1);
        const int sf0 = std::clamp((int) fp, 0, table_->frameCount - 1);
        const int sf1 = std::min(sf0 + 1, table_->frameCount - 1);
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
        const double dt0 = std::clamp(hz / sr, 0.0, 0.49);
        const int mip = levelFor(hz * bendRatio_, sr);
        const double target = poly ? 0.0 : 1.0;
        for (int i = 0; i < numSamples; ++i) {
            freeEnv_ += freeSm * (target - freeEnv_);
            const double dt = dt0 * (bendFrom + bendStep * i);
            int f0, f1; float fmix;
            pickFrames(posRamp_[(size_t) i], freeEnv_, f0, f1, fmix);
            const float* t0 = spectral ? morphMip_[mip] : table_->mip[f0][mip];
            const float* t1 = table_->mip[f1][mip];
            if (spectral) fmix = 0.0f;
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
        const double dt0 = std::clamp(v.hz / sr, 0.0, 0.49);
        const int mip = levelFor(v.hz * bendRatio_, sr);
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
            const double dt = dt0 * (bendFrom + bendStep * i);
            int f0, f1; float fmix;
            pickFrames(posRamp_[(size_t) i], v.env, f0, f1, fmix);
            const float* t0 = spectral ? morphMip_[mip] : table_->mip[f0][mip];
            const float* t1 = table_->mip[f1][mip];
            if (spectral) fmix = 0.0f;
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

    {
        const double driveCoef = 1.0 - std::exp(-1.0 / (0.008 * sr));
        for (int i = 0; i < numSamples; ++i) {
            driveSm_ += driveCoef * (drive - driveSm_);
            const float d = (float) driveSm_;
            const float g = 1.0f + 5.0f * d;
            const float wet = std::min(1.0f, d * 12.0f);
            auto sat = [g, wet, d](float x) {
                return d > 1.0e-4f ? x + wet * (std::tanh(x * g) - x) : x;
            };
            L[i] = driveOsL_.process(L[i], sat);
            if (R != L) R[i] = driveOsR_.process(R[i], sat);
        }
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

    for (int i = 0; i < numSamples; ++i) {
        L[i] *= level;
        if (R != L) R[i] *= level;
    }
}

}
