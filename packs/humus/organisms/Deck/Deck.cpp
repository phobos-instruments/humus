// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Deck/Deck.h"

#include <algorithm>
#include <utility>
#include <cmath>

#include "hum/dsp/SoundFileBuffer.h"
#include "hum/dsp/Interpolation.h"

namespace hum {

Deck::Deck() = default;
Deck::~Deck() = default;

namespace {
std::vector<float> buildPeaks(const juce::AudioBuffer<float>& buf, int buckets) {
    const int ch = buf.getNumChannels();
    const int64_t len = buf.getNumSamples();
    if (ch == 0 || len == 0) return {};
    std::vector<float> peaks((size_t) buckets, 0.0f);
    const double per = (double) len / (double) buckets;
    for (int b = 0; b < buckets; ++b) {
        const int64_t s0 = (int64_t) (b * per);
        const int64_t s1 = std::min(len, (int64_t) ((b + 1) * per));
        float peak = 0.0f;
        for (int64_t s = s0; s < s1; ++s)
            for (int c = 0; c < ch; ++c)
                peak = std::max(peak, std::abs(buf.getSample(c, (int) s)));
        peaks[(size_t) b] = std::min(1.0f, peak);
    }
    return peaks;
}
}

void Deck::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    maxBlock_ = std::max(64, maxBlock);
    stretch_.prepare(sampleRate_, 2, maxBlock_);
    { const float w[4] = {0, 0, 0, 0}; (void) sampleAt(w, 4, 1.0, Interp::Sinc); }
    stInL_.reserve((size_t) maxBlock_ * 4 + 16);
    stInR_.reserve((size_t) maxBlock_ * 4 + 16);
    stOutL_.reserve((size_t) maxBlock_ + 16);
    stOutR_.reserve((size_t) maxBlock_ + 16);
    loadFromFile(params.getText("File"));
    if (hasPending_.load()) applyPending();
    reset();
}

void Deck::reset() {
    readPos_ = 0.0;
    rate_ = 1.0;
    velocity_ = 0.0;
    prevSync_ = false;
    prevKeylock_ = false;
    rollActive_ = false;
    rollPhantom_ = 0.0;
    rollBeginReq_.store(false);
    rollEndReq_.store(false);
    stretch_.reset();
    tempo_ = 1.0;
    tempoPrimed_ = false;
    stretchInFrac_ = 0.0;
    declick_ = 1.0;
    scrubGain_ = 0.0;
    tc_.prepare(sampleRate_, params.get("TimecodeHz", 1000.0));
    prevTcHz_ = params.get("TimecodeHz", 1000.0);
}

void Deck::loadFromFile(const std::string& uri) {
    if (uri == loadedUri_) return;
    loadedUri_ = uri;
    juce::AudioBuffer<float> buf;
    double sr = sampleRate_;
    const bool ok = loadSoundFile(uri, buf, sr);
    const int64_t len = ok ? buf.getNumSamples() : 0;
    peaks_ = ok ? buildPeaks(buf, kWaveBuckets) : std::vector<float>{};
    {
        const juce::ScopedLock sl(loadLock_);
        pending_ = std::move(buf);
        pendingSr_ = ok ? sr : sampleRate_;
    }
    fileSr_.store(ok ? sr : sampleRate_);
    fileLen_.store(len);
    hasPending_.store(true);
}

void Deck::applyPending() {
    const juce::ScopedTryLock sl(loadLock_);
    if (!sl.isLocked()) return;
    std::swap(file_, pending_);
    fileSr_.store(pendingSr_);
    fileLen_.store(file_.getNumSamples());
    readPos_ = 0.0;
    velocity_ = 0.0;
    hasPending_.store(false);
    stretch_.reset();
}

double Deck::masterTempo() const {
    const bool active = params.get("Active", 1.0) >= 0.5;
    const bool master = params.get("Master", 0.0) >= 0.5;
    if (!active || !master) return 0.0;
    const double bpm = std::max(1.0, params.get("BPM", 120.0));
    const double pct = params.get("PitchPercent", 0.0) + bend_.load();
    return bpm * std::max(0.05, 1.0 + pct / 100.0);
}

void Deck::process(const float* const* in, int numIn,
                   float* const* out, int numOut,
                   int numSamples, const Transport& transport) {
    if (hasPending_.load()) applyPending();

    int64_t seek = seekReq_.exchange(-1);
    if (seek >= 0) { readPos_ = (double) seek; declick_ = 0.0; }

    if (rollBeginReq_.exchange(false)) { rollActive_ = true; rollPhantom_ = readPos_; }
    if (rollEndReq_.exchange(false)) {
        readPos_ = std::max(0.0, rollPhantom_);
        rollActive_ = false;
        declick_ = 0.0;
    }

    const bool active   = params.get("Active", 1.0) >= 0.5;
    const bool loop     = params.get("Loop", 0.0) >= 0.5;
    const bool sync     = params.get("Sync", 0.0) >= 0.5;
    const bool master   = params.get("Master", 0.0) >= 0.5;
    const double bend   = bend_.load();
    const double pitch  = std::max(0.05, 1.0 + (params.get("PitchPercent", 0.0) + bend) / 100.0);
    const double gridBpm = std::max(1.0, params.get("BPM", 120.0));
    const double vol    = params.get("Volume", 1.0);
    const double hostSr = transport.sampleRate() > 0.0 ? transport.sampleRate() : sampleRate_;
    const double fileSr = fileSr_.load();
    const double baseRate = fileSr > 0.0 ? fileSr / hostSr : 1.0;

    BeatGrid grid{gridBpm, (int64_t) params.get("GridOffset", 0.0)};

    double tempoFactor;
    bool tempoSnap = false;
    if (sync && !master) {
        tempoFactor = syncRate(gridBpm, transport.tempo()) * (1.0 + bend / 100.0);
        if (!prevSync_) {
            const double curBeat = grid.sampleToBeat(readPos_, fileSr);
            const double tgtFrac = transport.beats() - std::floor(transport.beats());
            readPos_ = grid.beatToSample(std::floor(curBeat) + tgtFrac, fileSr);
            if (readPos_ < 0.0) readPos_ = 0.0;
            declick_ = 0.0;
            tempoSnap = true;
        } else {
            double err = transport.beats() - grid.sampleToBeat(readPos_, fileSr);
            err -= std::round(err);
            tempoFactor *= (1.0 + 0.02 * juce::jlimit(-1.0, 1.0, err));
        }
    } else {
        tempoFactor = pitch;
    }
    prevSync_ = sync;
    if (!tempoPrimed_ || tempoSnap) { tempo_ = tempoFactor; tempoPrimed_ = true; }
    tempo_ += (tempoFactor - tempo_)
              * (1.0 - std::exp(-(double) numSamples / (0.08 * hostSr)));
    rate_ = baseRate * tempo_;
    effBpm_.store(gridBpm * tempo_);

    const int fileCh = file_.getNumChannels();
    const int64_t len = file_.getNumSamples();
    auto silence = [&](int n) { for (int c = 0; c < numOut; ++c) out[c][n] = 0.0f; };
    const Interp interp = params.get("HQ", 0.0) >= 0.5 ? Interp::Sinc : Interp::Cubic;
    auto frameAt = [&](double pos, int c) -> float {
        const int src = std::min(c, fileCh - 1);
        return sampleAt(file_.getReadPointer(src), len, pos, interp);
    };

    if (fileCh == 0 || len == 0) {
        for (int n = 0; n < numSamples; ++n) silence(n);
        velocity_ = 0.0;
        playPos_.store((int64_t) readPos_);
        return;
    }

    const bool scrubbing = scrubbing_.load();
    const double maxV = 8.0;

    const double dstep = 1.0 / (0.003 * hostSr);
    if (scrubbing) {
        const double target = juce::jlimit(0.0, (double) (len - 1), scrubTarget_.load());
        velocity_ = juce::jlimit(-maxV, maxV, (target - readPos_) / (double) numSamples);
        const double gateTarget = std::abs(velocity_) < 1.0e-3 ? 0.0 : vol;
        const double gcoef = 1.0 - std::exp(-1.0 / (0.002 * hostSr));
        for (int n = 0; n < numSamples; ++n) {
            scrubGain_ += (gateTarget - scrubGain_) * gcoef;
            declick_ = std::min(1.0, declick_ + dstep);
            const float g = (float) (scrubGain_ * declick_);
            for (int c = 0; c < numOut; ++c) out[c][n] = frameAt(readPos_, c) * g;
            readPos_ = juce::jlimit(0.0, (double) (len - 1), readPos_ + velocity_);
        }
        playPos_.store((int64_t) readPos_);
        return;
    }
    scrubGain_ = vol;

    const bool tcOn = params.get("Timecode", 0.0) >= 0.5;
    double tcTarget = 0.0;
    bool tcActive = false;
    if (tcOn && numIn >= 2 && in != nullptr && in[0] != nullptr && in[1] != nullptr) {
        const double hz = params.get("TimecodeHz", 1000.0);
        if (hz != prevTcHz_) { tc_.setNominal(hz); prevTcHz_ = hz; }
        for (int n = 0; n < numSamples; ++n) tc_.push(in[0][n], in[1][n]);
        tcActive = true;
        tcTarget = tc_.present() ? tc_.speed() * baseRate : 0.0;
    }

    const double motorTarget = !active ? 0.0 : tcActive ? tcTarget : rate_;
    if (!active && std::abs(velocity_) < 1.0e-4) {
        velocity_ = 0.0;
        for (int n = 0; n < numSamples; ++n) silence(n);
        playPos_.store((int64_t) readPos_);
        return;
    }

    const int64_t loopIn  = std::max<int64_t>(0, (int64_t) params.get("LoopIn", 0.0));
    const int64_t loopOut = (int64_t) params.get("LoopOut", 0.0);
    const bool haveLoop   = loop && loopOut > loopIn;

    const bool keylock = stretch_.available() && !tcActive
                         && params.get("Keylock", 0.0) >= 0.5;
    if (keylock && !prevKeylock_) stretch_.reset();
    prevKeylock_ = keylock;
    const bool settled = active && std::abs(velocity_ - rate_) < 1.0e-3;
    if (keylock && !settled) stretch_.reset();
    if (keylock && settled) {
        velocity_ = rate_;
        auto pullInput = [&](float& l, float& r) -> bool {
            if (haveLoop && readPos_ >= (double) loopOut) readPos_ = (double) loopIn;
            if (readPos_ >= (double) len) {
                if (!loop) return false;
                readPos_ = haveLoop ? (double) loopIn : 0.0;
            }
            l = frameAt(readPos_, 0); r = frameAt(readPos_, 1);
            readPos_ += baseRate;
            if (rollActive_) rollPhantom_ += baseRate;
            return true;
        };
        const int outN = numSamples;
        const double want = outN * std::max(0.05, tempo_) + stretchInFrac_;
        int inN = (int) want;
        stretchInFrac_ = want - inN;
        inN = std::max(1, std::min(inN, maxBlock_ * 4));
        stInL_.resize((size_t) inN); stInR_.resize((size_t) inN);
        for (int i = 0; i < inN; ++i) {
            float l = 0.0f, r = 0.0f;
            pullInput(l, r);
            stInL_[(size_t) i] = l; stInR_[(size_t) i] = r;
        }
        stOutL_.resize((size_t) outN); stOutR_.resize((size_t) outN);
        const float* inp[2]  = { stInL_.data(), stInR_.data() };
        float* outp[2] = { stOutL_.data(), stOutR_.data() };
        stretch_.process(inp, inN, outp, outN, tempo_);
        for (int n = 0; n < outN; ++n) {
            declick_ = std::min(1.0, declick_ + dstep);
            out[0][n] = stOutL_[(size_t) n] * (float) (vol * declick_);
            if (numOut > 1) out[1][n] = stOutR_[(size_t) n] * (float) (vol * declick_);
        }
        playPos_.store((int64_t) readPos_);
        return;
    }

    const double accel = 1.0 / ((tcActive ? 0.02 : 0.20) * hostSr);
    for (int n = 0; n < numSamples; ++n) {
        velocity_ += (motorTarget - velocity_) * accel;
        declick_ = std::min(1.0, declick_ + dstep);
        if (haveLoop && readPos_ >= (double) loopOut) readPos_ = (double) loopIn;
        if (readPos_ >= (double) len) {
            if (!loop) { velocity_ = 0.0; silence(n); continue; }
            readPos_ = haveLoop ? (double) loopIn : 0.0;
        }
        if (readPos_ < 0.0) readPos_ = 0.0;
        for (int c = 0; c < numOut; ++c)
            out[c][n] = frameAt(readPos_, c) * (float) (vol * declick_);
        readPos_ += velocity_;
        if (rollActive_) rollPhantom_ += velocity_;
    }
    playPos_.store((int64_t) readPos_);
}

}
