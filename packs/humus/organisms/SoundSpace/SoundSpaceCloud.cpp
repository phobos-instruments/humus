// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SoundSpace/SoundSpace.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

inline float frand(std::uint32_t& s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (float) (s & 0xFFFFFF) / (float) 0xFFFFFF;
}

template <typename Fetch>
void renderVoice(float* left, float* right, int numSamples, float master, double& pos, double rate,
                 int& remaining, int total, float ampL, float ampR, float rise, float hold,
                 Fetch fetch) {
    const float fall = std::max(0.0f, 1.0f - rise - hold);
    for (int n = 0; n < numSamples && remaining > 0; ++n, --remaining) {
        float sL = 0.0f, sR = 0.0f;
        if (!fetch(pos, sL, sR)) { remaining = 0; break; }
        const float t = 1.0f - (float) remaining / (float) total;
        float shape = 1.0f;
        if (t < rise) shape = rise > 0.0f ? 0.5f - 0.5f * std::cos(kPiF * t / rise) : 1.0f;
        else if (t > rise + hold) shape = fall > 0.0f ? 0.5f + 0.5f * std::cos(kPiF * (t - rise - hold) / fall) : 0.0f;
        const float g = shape * master;
        left[n]  += g * ampL * sL * 2.0f;
        right[n] += g * ampR * sR * 2.0f;
        pos += rate;
    }
}

}

SoundSpace::Picked SoundSpace::pickGrain(float x, float y, double rate) const {
    Picked picked;
    float bestD = 1.0e9f;
    if (corpus_)
        for (const auto& g : corpus_->grains) {
            const float dx = g.x - x, dy = g.y - y;
            const float d = dx * dx + dy * dy;
            if (d >= bestD) continue;
            bestD = d;
            picked.file = g.file;
            picked.start = g.start;
            picked.x = g.x;
            picked.y = g.y;
        }
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const std::int64_t written = liveWritten_.load(std::memory_order_relaxed);
    const std::int64_t oldest = written - (std::int64_t) (kLiveWindowSeconds * sr);
    bool liveWon = false;
    if (liveWanted())
        for (const auto& g : liveGrains_) {
            const std::int64_t start = g.start.load(std::memory_order_relaxed);
            if (start < 0 || start < oldest) continue;
            const float gx = g.x.load(std::memory_order_relaxed);
            const float gy = g.y.load(std::memory_order_relaxed);
            const float d = (gx - x) * (gx - x) + (gy - y) * (gy - y);
            if (d >= bestD) continue;
            bestD = d;
            liveWon = true;
            picked.x = gx;
            picked.y = gy;
            picked.file = SoundMapSource::kLiveFile;
            picked.start = (double) start;
        }
    if (picked.file < 0) return picked;
    if (liveWon) {
        picked.sourceRate = sr;
        const double gap = (double) written - picked.start;
        const double closing = params.get("Freeze", 0.0) >= 0.5 ? rate : rate - 1.0;
        picked.room = closing > 0.0 ? (int) std::min(1.0e9, gap / closing) - 2 : 1 << 30;
        return picked;
    }
    const auto& file = corpus_->files[(size_t) picked.file];
    picked.sourceRate = corpus_->rates[(size_t) picked.file];
    picked.room = (int) ((file.getNumSamples() - picked.start) / std::max(0.01, rate * picked.sourceRate / sr)) - 2;
    if (file.getNumSamples() == 0) picked.file = -1;
    return picked;
}

double SoundSpace::between(double low, double high) {
    return high <= low ? low : low + (high - low) * (double) frand(rng_);
}

double SoundSpace::between(double low, double high, double shared, double apart) {
    if (high <= low) return low;
    const double own = (double) frand(rng_);
    return low + (high - low) * std::clamp(shared + apart * (own - shared), 0.0, 1.0);
}

int SoundSpace::voicesSounding() const {
    int sounding = 0;
    for (const auto& v : voices_) sounding += v.file >= 0 ? 1 : 0;
    return sounding;
}

void SoundSpace::spawnGrain() {
    const int ceiling = std::clamp((int) std::lround(params.get("MaxGrains", 64.0)), 1, kMaxGrainVoices);
    if (voicesSounding() >= ceiling) return;
    Voice* slot = nullptr;
    for (auto& v : voices_)
        if (v.file < 0) { slot = &v; break; }
    if (!slot) return;

    const double shared = (double) frand(rng_);
    const double apart = std::clamp(params.get("Decorrelation", 1.0), 0.0, 1.0);
    const float spray = (float) params.get("Spray", 0.12);
    const float tx = (float) params.get("X", 0.5) + spray * (frand(rng_) * 2.0f - 1.0f);
    const float ty = (float) params.get("Y", 0.5) + spray * (frand(rng_) * 2.0f - 1.0f);
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double semitones = between(params.get("Pitch", 0.0), params.getMax("Pitch", 0.0), shared, apart);
    const double transpose = std::pow(2.0, semitones / 12.0);

    const Picked picked = pickGrain(tx, ty, transpose);
    if (picked.file < 0) return;

    const double ms = between(params.get("GrainSize", 120.0), params.getMax("GrainSize", 120.0), shared, apart);
    const int total = std::clamp((int) (ms * 0.001 * sr), 32, std::max(32, picked.room));

    const float spread = (float) params.get("Spread", 0.4);
    const double panRoll = apart >= 1.0 ? (double) frand(rng_) : shared + apart * ((double) frand(rng_) - shared);
    const float pan = 0.5f + spread * (float) (std::clamp(panRoll, 0.0, 1.0) - 0.5);
    slot->file = picked.file;
    slot->pos = picked.start;
    slot->rate = transpose * (picked.sourceRate / sr);
    slot->total = slot->remaining = total;
    slot->ampL = 1.0f - pan;
    slot->ampR = pan;
    const double hold = between(params.get("Shape", 0.0), params.getMax("Shape", 0.0), shared, apart);
    const double skew = between(params.get("Skew", 0.0), params.getMax("Skew", 0.0), shared, apart);
    slot->hold = (float) std::clamp(hold, 0.0, 0.94);
    slot->rise = (float) ((1.0 - slot->hold) * std::clamp(0.5 - 0.48 * skew, 0.02, 0.98));
    publishFlash(picked, (float) (total / sr));
}

void SoundSpace::publishFlash(const Picked& picked, float seconds) {
    if (++flashSerial_ == 0) ++flashSerial_;
    const std::uint32_t serial = flashSerial_;
    auto& slot = flashes_[serial % kFlashRing];
    slot.serial.store(0, std::memory_order_release);
    slot.x.store(picked.x, std::memory_order_relaxed);
    slot.y.store(picked.y, std::memory_order_relaxed);
    slot.seconds.store(seconds, std::memory_order_relaxed);
    slot.file.store(picked.file, std::memory_order_relaxed);
    slot.serial.store(serial, std::memory_order_release);
}

int SoundSpace::recentGrains(Flash* out, int capacity) const {
    int n = 0;
    for (const auto& slot : flashes_) {
        if (n >= capacity) break;
        const std::uint32_t before = slot.serial.load(std::memory_order_acquire);
        if (before == 0) continue;
        Flash f;
        f.x = slot.x.load(std::memory_order_relaxed);
        f.y = slot.y.load(std::memory_order_relaxed);
        f.seconds = slot.seconds.load(std::memory_order_relaxed);
        f.file = slot.file.load(std::memory_order_relaxed);
        f.serial = before;
        if (slot.serial.load(std::memory_order_acquire) != before) continue;
        out[n++] = f;
    }
    return n;
}

void SoundSpace::renderAdd(float* left, float* right, int numSamples) {
    const float master = mute_ ? 0.0f : (float) params.get("Gain", 1.0);
    const auto ring = (std::int64_t) liveLeft_.size();
    const std::int64_t written = liveWritten_.load(std::memory_order_relaxed);
    for (auto& v : voices_) {
        if (v.file < 0) continue;
        if (v.file == SoundMapSource::kLiveFile) {
            renderVoice(left, right, numSamples, master, v.pos, v.rate, v.remaining, v.total, v.ampL, v.ampR, v.rise, v.hold,
                        [&](double pos, float& sL, float& sR) {
                const auto i0 = (std::int64_t) pos;
                if (ring == 0 || i0 + 1 >= written || i0 <= written - ring) return false;
                const float frac = (float) (pos - (double) i0);
                const auto a = (size_t) (i0 % ring), b = (size_t) ((i0 + 1) % ring);
                sL = liveLeft_[a] + frac * (liveLeft_[b] - liveLeft_[a]);
                sR = liveRight_[a] + frac * (liveRight_[b] - liveRight_[a]);
                return true;
            });
        } else if (corpus_) {
            const auto& buf = corpus_->files[(size_t) v.file];
            const int len = buf.getNumSamples();
            const float* srcL = buf.getReadPointer(0);
            const float* srcR = buf.getReadPointer(buf.getNumChannels() > 1 ? 1 : 0);
            renderVoice(left, right, numSamples, master, v.pos, v.rate, v.remaining, v.total, v.ampL, v.ampR, v.rise, v.hold,
                        [&](double pos, float& sL, float& sR) {
                const int i0 = (int) pos;
                if (i0 >= len - 1) return false;
                const float frac = (float) (pos - i0);
                sL = srcL[i0] + frac * (srcL[i0 + 1] - srcL[i0]);
                sR = srcR[i0] + frac * (srcR[i0 + 1] - srcR[i0]);
                return true;
            });
        } else {
            v.remaining = 0;
        }
        if (v.remaining <= 0) v = Voice{};
    }
}

}
