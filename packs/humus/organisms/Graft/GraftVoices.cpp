// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Graft/Graft.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace hum {

namespace {

constexpr double kMinEnvSeconds = 0.0005;
constexpr double kSwapBlendSeconds = 0.012;
constexpr double kAuditionReleaseMs = 12.0;

float envelopeRate(double milliseconds, double sampleRate) {
    return (float) (1.0 / std::max(kMinEnvSeconds, milliseconds * 0.001) / sampleRate);
}

}

void Graft::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
    blendSpan_ = std::max(1, (int) std::lround(kSwapBlendSeconds * sampleRate_));
    restitchNow();
    adoptPending();
    reset();
}

void Graft::reset() {
    for (auto& v : voices_) v = Voice{};
    drone_ = Voice{};
    audition_ = Voice{};
    bend_.reset();
    bendRatio_ = 1.0;
    blendLeft_ = 0;
    auditionWanted_.store(-1, std::memory_order_relaxed);
    stagedCount_ = 0;
    playing_.store(-1, std::memory_order_relaxed);
}

void Graft::adoptPending() {
    if (!hasPending_.load(std::memory_order_acquire)) return;
    const juce::SpinLock::ScopedTryLockType lock(swap_);
    if (!lock.isLocked()) return;
    const bool wasSounding = buf_.getNumSamples() > 1;
    std::swap(buf_, pendingBuf_);
    std::swap(bounds_, pendingBounds_);
    hasPending_.store(false, std::memory_order_release);
    if (!wasSounding || buf_.getNumSamples() < 2) { blendLeft_ = 0; return; }

    std::swap(leavingBuf_, pendingBuf_);
    blendLeft_ = blendSpan_;
    auto arm = [](Voice& v) {
        v.leaving = v.pos;
        v.blending = v.active;
    };
    arm(drone_);
    arm(audition_);
    for (auto& v : voices_) arm(v);
}

void Graft::deliverMidi(int, const MidiEvent* events, int count) {
    for (int i = 0; i < count && stagedCount_ < (int) staged_.size(); ++i)
        staged_[(size_t) stagedCount_++] = events[i];
}

void Graft::pushLiveMidi(const MidiEvent& e) {
    const std::lock_guard<std::mutex> lock(liveLock_);
    if (liveCount_ < (int) liveQ_.size()) liveQ_[(size_t) liveCount_++] = e;
}

void Graft::startVoice(Voice& v, int note, float velocity, bool drone) {
    const int len = buf_.getNumSamples();
    if (len < 2) { v.active = false; return; }
    const double speed = std::clamp(params.get("Speed", 1.0), 0.25, 4.0);
    const double start = std::clamp(params.get("Start", 0.0), 0.0, 1.0);
    const bool backwards = params.get("Reverse", 0.0) >= 0.5;

    v.step = std::pow(2.0, (note - kRootNote) / kSemitonesPerOctave) * speed * (backwards ? -1.0 : 1.0);
    v.pos = backwards ? (double) (len - 2) : start * (double) (len - 2);
    v.env = 0.0f;
    v.gain = velocity;
    v.note = note;
    v.stage = 0;
    v.active = true;
    v.drone = drone;
}

void Graft::handleEvent(const MidiEvent& e) {
    if (e.size < 3) return;
    if (bend_.apply(e)) { bendRatio_ = bend_.ratio(bendRangeOf(params)); return; }
    const int status = e.data[0] & 0xF0;
    const int note = e.data[1];
    const int velocity = e.data[2];
    if (status == 0x80 || (status == 0x90 && velocity == 0)) {
        for (auto& v : voices_)
            if (v.active && v.note == note) v.stage = 3;
        return;
    }
    if (status != 0x90) return;

    const int wanted = (int) std::clamp(params.get("Voices", 8.0), 1.0, (double) kMaxVoices);
    Voice* pick = nullptr;
    for (int i = 0; i < wanted; ++i)
        if (!voices_[(size_t) i].active) { pick = &voices_[(size_t) i]; break; }
    if (pick == nullptr) pick = &voices_[0];
    startVoice(*pick, note, (float) velocity / kMidiMaxF, false);
}

void Graft::advanceEnvelope(Voice& v, float attack, float decay, float sustain, float release) {
    if (v.releaseOverride > 0.0f) release = v.releaseOverride;
    switch (v.stage) {
        case 0:
            v.env += attack;
            if (v.env >= 1.0f) { v.env = 1.0f; v.stage = 1; }
            break;
        case 1:
            v.env -= decay;
            if (v.env <= sustain) { v.env = sustain; v.stage = 2; }
            break;
        case 2:
            v.env = sustain;
            if (sustain <= 0.0f && !v.drone) v.active = false;
            break;
        default:
            v.env -= release;
            if (v.env <= 0.0f) { v.env = 0.0f; v.active = false; }
            break;
    }
}

void Graft::renderAdd(float* left, float* right, int from, int count) {
    const int len = buf_.getNumSamples();
    if (len < 2) return;
    const float level = (float) std::clamp(params.get("Level", 1.0), 0.0, 2.0);
    const bool loop = params.get("Loop", 0.0) >= 0.5;
    const double start = std::clamp(params.get("Start", 0.0), 0.0, 1.0) * (double) (len - 2);
    const float sustain = (float) std::clamp(params.get("Sustain", 1.0), 0.0, 1.0);
    const float attack = envelopeRate(params.get("Attack", 4.0), sampleRate_);
    const float decay = envelopeRate(params.get("Decay", 200.0), sampleRate_) * (1.0f - sustain);
    const float release = envelopeRate(params.get("Release", 200.0), sampleRate_);
    const float* srcL = buf_.getReadPointer(0);
    const float* srcR = buf_.getNumChannels() > 1 ? buf_.getReadPointer(1) : srcL;

    const int wasLen = leavingBuf_.getNumSamples();
    const bool blending = blendLeft_ > 0 && wasLen > 1;
    const float* wasL = blending ? leavingBuf_.getReadPointer(0) : nullptr;
    const float* wasR = blending && leavingBuf_.getNumChannels() > 1
                            ? leavingBuf_.getReadPointer(1) : wasL;

    auto tap = [](const float* x, int n, double at) {
        const int i0 = std::min((int) at, n - 2);
        return x[i0] + (x[i0 + 1] - x[i0]) * (float) (at - (double) i0);
    };

    auto play = [&](Voice& v) {
        if (!v.active) return;
        for (int i = from; i < from + count && v.active; ++i) {
            if (v.stop > 0.0 && v.pos >= v.stop) {
                v.stop = 0.0;
                v.stage = 3;
            }
            if (v.pos >= (double) (len - 1) || v.pos < 0.0) {
                if (!loop && !v.drone) { v.active = false; break; }
                v.pos = v.step >= 0.0 ? start : (double) (len - 2);
            }
            advanceEnvelope(v, attack, decay, sustain, release);

            const int leftToBlend = blendLeft_ - (i - from);
            const bool mixing = v.blending && blending && leftToBlend > 0;
            float arriving = 1.0f, going = 0.0f;
            if (mixing) {
                const float t = 1.0f - (float) leftToBlend / (float) blendSpan_;
                arriving = fadeGain(t, kFadeEqualPower);
                going = fadeGain(1.0f - t, kFadeEqualPower);
            }

            float l = tap(srcL, len, v.pos) * arriving;
            float r = tap(srcR, len, v.pos) * arriving;
            if (mixing && v.leaving >= 0.0 && v.leaving < (double) (wasLen - 1)) {
                l += tap(wasL, wasLen, v.leaving) * going;
                r += tap(wasR, wasLen, v.leaving) * going;
            }

            const float g = v.env * v.gain * level;
            left[i] += l * g;
            if (right != nullptr) right[i] += r * g;
            v.pos += v.step * bendRatio_;
            v.leaving += v.step * bendRatio_;
        }
    };
    play(drone_);
    play(audition_);
    for (auto& v : voices_) play(v);
}

void Graft::takeAudition() {
    const int wanted = auditionWanted_.exchange(-1, std::memory_order_relaxed);
    const int len = buf_.getNumSamples();
    if (wanted < 0 || len < 2 || bounds_.empty()) return;
    const int slot = std::clamp(wanted, 0, (int) bounds_.size() - 1);
    const double from = (double) bounds_[(size_t) slot] * (double) (len - 2);
    const double to = slot + 1 < (int) bounds_.size()
                          ? (double) bounds_[(size_t) slot + 1] * (double) (len - 1)
                          : (double) (len - 1);

    audition_ = Voice{};
    audition_.pos = from;
    audition_.stop = std::max(from + 16.0, to);
    audition_.step = std::clamp(params.get("Speed", 1.0), 0.25, 4.0);
    audition_.gain = 1.0f;
    audition_.releaseOverride = envelopeRate(kAuditionReleaseMs, sampleRate_);
    audition_.active = true;
}

void Graft::process(const float* const*, int, float* const* out, int numOut, int numSamples,
                    const Transport& transport) {
    if (numOut < 1) return;
    adoptPending();
    lastTempo_.store(transport.tempo(), std::memory_order_relaxed);
    lastBeatsPerBar_.store(transport.beatsPerBar(), std::memory_order_relaxed);

    float* left = out[0];
    float* right = numOut > 1 ? out[1] : nullptr;
    std::memset(left, 0, sizeof(float) * (size_t) numSamples);
    if (right != nullptr) std::memset(right, 0, sizeof(float) * (size_t) numSamples);
    for (int c = 2; c < numOut; ++c)
        std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);

    {
        std::unique_lock<std::mutex> lock(liveLock_, std::try_to_lock);
        if (lock.owns_lock()) {
            for (int i = 0; i < liveCount_ && stagedCount_ < (int) staged_.size(); ++i)
                staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
            liveCount_ = 0;
        }
    }
    for (int i = 0; i < stagedCount_; ++i) handleEvent(staged_[(size_t) i]);
    stagedCount_ = 0;
    takeAudition();

    const bool wantDrone = params.get("Play", 0.0) >= 0.5 && buf_.getNumSamples() > 1;
    if (wantDrone && !drone_.active)
        startVoice(drone_, kRootNote, 1.0f, true);
    else if (!wantDrone && drone_.active && drone_.stage != 3)
        drone_.stage = 3;
    else if (wantDrone && drone_.stage == 3)
        drone_.stage = 2;

    renderAdd(left, right, 0, numSamples);
    blendLeft_ = std::max(0, blendLeft_ - numSamples);
    if (blendLeft_ == 0) {
        drone_.blending = false;
        audition_.blending = false;
        for (auto& v : voices_) v.blending = false;
    }

    int lit = -1;
    const int len = buf_.getNumSamples();
    if (len > 1 && !bounds_.empty()) {
        const Voice* sounding = drone_.active ? &drone_ : nullptr;
        for (const auto& v : voices_)
            if (v.active) { sounding = &v; break; }
        if (sounding != nullptr) {
            const float where = (float) (sounding->pos / (double) len);
            for (int i = 0; i + 1 < (int) bounds_.size(); ++i)
                if (bounds_[(size_t) i] <= where) lit = i;
        }
    }
    playing_.store(lit, std::memory_order_relaxed);
}

}
