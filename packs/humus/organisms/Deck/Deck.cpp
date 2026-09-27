// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Deck/Deck.h"

#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

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

struct PeaksMemo {
    std::string key;
    std::vector<float> peaks;
};

constexpr std::size_t kPeaksMemoSlots = 8;

std::vector<float> peaksFor(const std::string& uri, int track, const juce::AudioBuffer<float>& buf,
                            int buckets) {
    const auto key = soundFileStamp(juce::File(juce::String(juce::CharPointer_UTF8(uri.c_str()))),
                                    track);
    static juce::CriticalSection lock;
    static std::vector<PeaksMemo> memo;
    {
        const juce::ScopedLock sl(lock);
        for (const auto& m : memo)
            if (m.key == key) return m.peaks;
    }
    auto peaks = buildPeaks(buf, buckets);
    const juce::ScopedLock sl(lock);
    if (memo.size() >= kPeaksMemoSlots) memo.erase(memo.begin());
    memo.push_back({key, peaks});
    return peaks;
}

std::string loadFaultFor(const std::string& uri, bool ok) {
    if (uri.empty() || ok) return {};
    const juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    if (!f.existsAsFile()) return "the file is missing";
    return "no sound in it could be read";
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
    if (defer_) warmFromFile(audioPathFor({}));
    else loadFromFile(audioPathFor({}));
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
    blockStartPos_ = 0.0;
    playSpeed_.store(0.0);
    rollBeginReq_.store(false);
    rollEndReq_.store(false);
    stretch_.reset();
    tempo_ = 1.0;
    tempoPrimed_ = false;
    stretchInFrac_ = 0.0;
    declick_ = 1.0;
    scrubGain_ = 0.0;
    vinylRate_ = sampleRate_;
    vinyl_.prepare(vinylRate_, {});
}

void Deck::warmFromFile(const std::string& uri) {
    if (uri.empty() || uri == loadedUri_) return;
    if (warmSoundFile(uri).audio == nullptr) {
        loadProgress_.store(0.0f);
        return;
    }
    loadFromFile(uri);
}

bool Deck::startTake(const std::string& wavPath, double sampleRate) {
    if (writer_.active()) return false;
    if (!writer_.start(wavPath, 2, sampleRate)) return false;
    takeLen_.store(0, std::memory_order_relaxed);
    takeStartBeat_.store(-1.0, std::memory_order_relaxed);
    takeActive_.store(true, std::memory_order_relaxed);
    return true;
}

void Deck::stopTake() {
    takeActive_.store(false, std::memory_order_relaxed);
    writer_.stop();
}

void Deck::captureTake(const float* const* in, int numIn, int numSamples, const Transport& t) {
    if (!takeActive_.load(std::memory_order_relaxed)) return;
    if (recLen_.load(std::memory_order_relaxed) > 0 && params.get("Active", 0.0) < 0.5) return;
    if (numIn <= 0 || in == nullptr || in[0] == nullptr) return;
    if (takeLen_.load(std::memory_order_relaxed) == 0)
        takeStartBeat_.store(t.beats(), std::memory_order_relaxed);
    const float* chans[2] = {in[0], numIn > 1 && in[1] != nullptr ? in[1] : in[0]};
    writer_.write(chans, numSamples);
    takeLen_.fetch_add(numSamples, std::memory_order_relaxed);
}

std::string Deck::audioPathFor(const std::string& uri) const {
    const std::string footage = params.getText("File");
    const std::string sound = params.getText("FileSound");
    if (!sound.empty() && !footage.empty()) return sound;
    return !uri.empty() ? uri : footage;
}

void Deck::notePicture() {
    auto footage = params.getText("File");
    const juce::ScopedLock sl(loadLock_);
    picture_ = std::move(footage);
}

void Deck::adoptStream(const SharedSound& sound, const SoundFill& filled, bool sameSound,
                       double wasSr) {
    if (sound.audio == nullptr || sound.audio->getNumSamples() <= 0) return;
    const double sr = sound.info.sampleRate;
    {
        const juce::ScopedLock sl(loadLock_);
        pending_ = sound.audio;
        pendingFill_ = filled;
        pendingSr_ = sr;
    }
    keepsPosition_.store(sameSound);
    positionScale_.store(sameSound && wasSr > 0.0 && sr > 0.0 ? sr / wasSr : 1.0);
    streaming_.store(true);
    fileSr_.store(sr);
    fileLen_.store(sound.audio->getNumSamples());
    hasPending_.store(true);
    if (!sameSound) videoLaunch_.fetch_add(1);
    notePicture();
}

void Deck::loadFromFile(const std::string& uri) {
    const std::string path = audioPathFor(uri);
    const int track = (int) params.get("AudioTrack", 0.0);
    if (path == loadedUri_ && track == loadedTrack_) { notePicture(); return; }
    const bool sameSound = !playingUri_.empty() && path == playingUri_;
    const double wasSr = fileSr_.load();
    loadedUri_ = path;
    loadedTrack_ = track;
    silentLen_.store(0);
    abandon_.store(false);
    loadProgress_.store(path.empty() ? 1.0f : 0.0f);
    auto names = soundFileTracks(path);
    SoundFill handed;
    auto sound = loadSharedSoundFile(
        path, [this](float done) { loadProgress_.store(done * 0.97f); }, 0.0,
        [this] { return abandon_.load(); }, track,
        [&](const SharedSound& part, const SoundFill& filled) {
            handed = filled;
            adoptStream(part, filled, sameSound, wasSr);
        });
    if (abandon_.load()) {
        streaming_.store(false);
        if (handed != nullptr) fileLen_.store(handed->load());
        const juce::ScopedLock sl(loadLock_);
        fault_.clear();
        loadedUri_.clear();
        loadedTrack_ = -1;
        loadProgress_.store(1.0f);
        return;
    }
    const bool streamed = handed != nullptr;
    if (!streamed) {
        keepsPosition_.store(sameSound);
        positionScale_.store(1.0);
    }
    const bool ok = sound.audio != nullptr && sound.audio->getNumSamples() > 0;
    const double sr = ok ? sound.info.sampleRate : sampleRate_;
    const int64_t len = ok ? sound.audio->getNumSamples() : 0;
    auto peaks = ok ? peaksFor(path, track, *sound.audio, kWaveBuckets) : std::vector<float>{};
    auto fault = loadFaultFor(path, ok);
    {
        const juce::ScopedLock sl(loadLock_);
        tracks_ = std::move(names);
        fault_ = std::move(fault);
        peaks_ = std::move(peaks);
        if (!streamed) {
            pending_ = ok ? sound.audio : SharedAudio{};
            pendingFill_ = {};
            pendingSr_ = sr;
        }
    }
    streaming_.store(false);
    playingUri_ = path;
    loadProgress_.store(1.0f);
    if (streamed) return;
    if (sameSound && wasSr > 0.0 && sr > 0.0) positionScale_.store(sr / wasSr);
    notePicture();
    fileSr_.store(sr);
    fileLen_.store(len);
    hasPending_.store(true);
    if (!sameSound) videoLaunch_.fetch_add(1);
}

std::vector<float> Deck::wavePeaksBetween(int64_t from, int64_t to, int buckets) const {
    SharedAudio held;
    SoundFill fill;
    {
        const juce::ScopedLock sl(loadLock_);
        held = file_;
        fill = fill_;
    }
    if (held == nullptr || buckets <= 0) return {};
    const int64_t len = held->getNumSamples();
    const int channels = held->getNumChannels();
    if (len <= 0 || channels <= 0) return {};
    const int64_t ready = fill != nullptr ? std::min<int64_t>(len, fill->load()) : len;
    from = from < 0 ? 0 : from;
    to = to > ready ? ready : to;
    if (to <= from) return {};

    std::vector<float> out((size_t) buckets, 0.0f);
    const double per = (double) (to - from) / (double) buckets;
    for (int b = 0; b < buckets; ++b) {
        const int64_t lo = from + (int64_t) (per * b);
        const int64_t hi = std::min(to, from + (int64_t) (per * (b + 1)));
        if (hi <= lo) continue;
        const int64_t stride = std::max<int64_t>(1, (hi - lo) / kBucketProbes);
        float peak = 0.0f;
        for (int c = 0; c < channels; ++c) {
            const float* samples = held->getReadPointer(c);
            for (int64_t i = lo; i < hi; i += stride)
                peak = std::max(peak, std::abs(samples[i]));
        }
        out[(size_t) b] = peak;
    }
    return out;
}

void Deck::rolled() {
    const int64_t len = fileLen_.load();
    if (len <= 1) return;
    const double fraction = (double) std::rand() / ((double) RAND_MAX + 1.0);
    double at = fraction * (double) (len - 1);
    if (params.get("Quantize", 1.0) >= 0.5) {
        const double fileSr = fileSr_.load();
        if (fileSr > 0.0) {
            const BeatGrid grid{std::max(1.0, params.get("BPM", 120.0)),
                                (int64_t) params.get("GridOffset", 0.0)};
            at = grid.nearestBeatSample(at, fileSr);
        }
    }
    seekReq_.store((int64_t) std::clamp(at, 0.0, (double) (len - 1)));
}

void Deck::publishPosition(int numSamples, double hostSr) {
    playPos_.store((int64_t) readPos_);
    const double wall = (double) numSamples / std::max(1.0, hostSr);
    const double media = (readPos_ - blockStartPos_) / std::max(1.0, fileSr_.load());
    playSpeed_.store(numSamples > 0 ? juce::jlimit(-8.0, 8.0, media / wall) : 0.0);
}

void Deck::applyPending() {
    const juce::ScopedTryLock sl(loadLock_);
    if (!sl.isLocked()) return;
    const double held = readPos_ * positionScale_.load();
    std::swap(file_, pending_);
    std::swap(fill_, pendingFill_);
    fileSr_.store(pendingSr_);
    const int64_t heard = file_ ? file_->getNumSamples() : 0;
    const int64_t silent = silentLen_.load(std::memory_order_relaxed);
    fileLen_.store(heard > 0 ? heard : silent);
    const int64_t len = fileLen_.load();
    if (keepsPosition_.load() && len > 0) {
        readPos_ = std::min(held, (double) (len - 1));
        declick_ = 0.0;
    } else {
        readPos_ = 0.0;
    }
    keepsPosition_.store(false);
    velocity_ = 0.0;
    hasPending_.store(false);
    stretch_.reset();
}

void Deck::process(const float* const* in, int numIn,
                   float* const* out, int numOut,
                   int numSamples, const Transport& transport) {
    for (int ch = 0; ch < kInletChans; ++ch) {
        float peak = 0.0f;
        if (in != nullptr && ch < numIn && in[ch] != nullptr)
            for (int i = 0; i < numSamples; ++i) peak = std::max(peak, std::abs(in[ch][i]));
        const float was = inPeak_[(size_t) ch].load(std::memory_order_relaxed) * kInletDecay;
        inPeak_[(size_t) ch].store(std::max(peak, was), std::memory_order_relaxed);
    }
    const bool taking = params.get("Record", 0.0) >= 0.5;
    if (!taking) recLen_.store(0, std::memory_order_relaxed);
    else if (params.get("Active", 0.0) >= 0.5)
        recLen_.fetch_add(numSamples, std::memory_order_relaxed);
    captureTake(in, numIn, numSamples, transport);
    if (hasPending_.load()) applyPending();

    int64_t seek = seekReq_.exchange(-1);
    if (seek >= 0) { readPos_ = (double) seek; declick_ = 0.0; }

    if (rollBeginReq_.exchange(false)) { rollActive_ = true; rollPhantom_ = readPos_; }
    if (rollEndReq_.exchange(false)) {
        readPos_ = std::max(0.0, rollPhantom_);
        rollActive_ = false;
        declick_ = 0.0;
    }

    blockStartPos_ = readPos_;

    const bool active   = params.get("Active", 1.0) >= 0.5;
    const bool loop     = params.get("Loop", 0.0) >= 0.5;
    const bool sync     = params.get("Sync", 0.0) >= 0.5;
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
    if (sync) {
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

    const int fileCh = file_ ? file_->getNumChannels() : 0;
    const int64_t audioLen = file_ ? file_->getNumSamples() : 0;
    const bool filling = streaming_.load(std::memory_order_relaxed);
    const int64_t ready = fill_ != nullptr
        ? std::min<int64_t>(audioLen, fill_->load(std::memory_order_relaxed))
        : audioLen;
    const int64_t have = filling ? audioLen : ready;
    const int64_t len = have > 0 ? have : silentLen_.load();
    auto silence = [&](int n) { for (int c = 0; c < numOut; ++c) out[c][n] = 0.0f; };
    constexpr Interp interp = Interp::Sinc;
    auto frameAt = [&](double pos, int c) -> float {
        if (fileCh == 0 || ready == 0) return 0.0f;
        const int src = std::min(c, fileCh - 1);
        return sampleAt(file_->getReadPointer(src), ready, pos, interp);
    };

    if (len == 0) {
        for (int n = 0; n < numSamples; ++n) silence(n);
        velocity_ = 0.0;
        publishPosition(numSamples, hostSr);
        return;
    }

    if (filling && !soundReadyFor(readPos_, ready, numSamples, rate_)) {
        for (int n = 0; n < numSamples; ++n) silence(n);
        velocity_ = 0.0;
        declick_ = 0.0;
        publishPosition(numSamples, hostSr);
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
        publishPosition(numSamples, hostSr);
        return;
    }
    scrubGain_ = vol;

    const bool tcOn = params.get("Timecode", 0.0) >= 0.5;
    double tcTarget = 0.0;
    bool tcActive = false;
    if (tcOn && numIn >= 2 && in != nullptr && in[0] != nullptr && in[1] != nullptr) {
        if (std::abs(hostSr - vinylRate_) > 1.0) {
            vinylRate_ = hostSr;
            vinyl_.prepare(vinylRate_, {});
        }
        const bool flip = params.get("TimecodeFlip", 0.0) >= 0.5;
        tcL_.resize((size_t) numSamples);
        tcR_.resize((size_t) numSamples);
        double power = 0.0;
        for (int n = 0; n < numSamples; ++n) {
            const float l = in[flip ? 1 : 0][n], r = in[flip ? 0 : 1][n];
            tcL_[(size_t) n] = l;
            tcR_[(size_t) n] = r;
            power += (double) l * l + (double) r * r;
        }
        {
            unsigned w = fieldAt_.load(std::memory_order_relaxed);
            double sll = 0.0, srr = 0.0, slr = 0.0;
            for (int n = 0; n < numSamples; ++n) {
                const float l = tcL_[(size_t) n], r = tcR_[(size_t) n];
                field_[2 * w] = l;
                field_[2 * w + 1] = r;
                w = (w + 1) % (unsigned) StereoFieldSource::kFieldPairs;
                sll += (double) l * l;
                srr += (double) r * r;
                slr += (double) l * r;
            }
            fieldAt_.store(w, std::memory_order_relaxed);
            const double denom = std::sqrt(sll * srr);
            if (denom > 1e-12) fieldCorr_.store((float) (slr / denom), std::memory_order_relaxed);
            fieldStamp_.fetch_add(1, std::memory_order_relaxed);
        }
        vinyl_.push(tcL_.data(), tcR_.data(), numSamples);
        const auto reading = vinyl_.read();
        tcActive = true;
        tcTarget = reading.toneArriving ? reading.speed * baseRate : 0.0;
        tcLevel_.store((float) std::sqrt(power / std::max(1, 2 * numSamples)));
        tcPresent_.store(reading.toneArriving);
        tcSpeed_.store((float) reading.speed);
        tcCarrier_.store((float) reading.restHz);
        const auto all = ControlVinyl::formats();
        const auto settled = vinyl_.format();
        int index = -1;
        for (size_t i = 0; i < all.size(); ++i)
            if (all[i] == settled) index = (int) i;
        tcFormat_.store(index);
    }
    tcOn_.store(tcOn);
    if (!tcActive) {
        tcPresent_.store(false);
        tcLevel_.store(0.0f);
        tcSpeed_.store(0.0f);
    }

    const double motorTarget = !active ? 0.0 : tcActive ? tcTarget : rate_;
    const bool stopping = !active && !tcActive;
    if (stopping) velocity_ = 0.0;
    if (!active && (stopping ? declick_ <= 0.0 : std::abs(velocity_) < 1.0e-4)) {
        velocity_ = 0.0;
        declick_ = 0.0;
        for (int n = 0; n < numSamples; ++n) silence(n);
        publishPosition(numSamples, hostSr);
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
        publishPosition(numSamples, hostSr);
        return;
    }

    const double accel = tcActive ? 1.0 / (0.02 * hostSr) : 1.0;
    for (int n = 0; n < numSamples; ++n) {
        velocity_ += (motorTarget - velocity_) * accel;
        declick_ = stopping ? std::max(0.0, declick_ - dstep)
                            : std::min(1.0, declick_ + dstep);
        if (haveLoop && readPos_ >= (double) loopOut) readPos_ = (double) loopIn;
        if (readPos_ >= (double) len) {
            if (loop) readPos_ = haveLoop ? (double) loopIn : 0.0;
            else if (!tcActive) { velocity_ = 0.0; silence(n); continue; }
        }
        if (readPos_ < 0.0) readPos_ = 0.0;
        if (readPos_ >= (double) len) silence(n);
        else
            for (int c = 0; c < numOut; ++c)
                out[c][n] = frameAt(readPos_, c) * (float) (vol * declick_);
        readPos_ = std::min(readPos_ + velocity_, (double) len);
        if (rollActive_) rollPhantom_ += velocity_;
    }
    publishPosition(numSamples, hostSr);
}

}
