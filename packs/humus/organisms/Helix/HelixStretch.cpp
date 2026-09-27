// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Helix/HelixStretch.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hum {

namespace {

constexpr long kPlaySeed = 0x4E11C5L;
constexpr long kDubSeed = 0x4E11C6L;

double wrapPos(double x, std::int64_t len) {
    const double n = (double) len;
    if (x >= 0.0 && x < n) return x;
    x = std::fmod(x, n);
    if (x < 0.0) x += n;
    return x >= n ? 0.0 : x;
}

double wrapDelta(double d, std::int64_t len) {
    const double n = (double) len;
    d = std::fmod(d, n);
    if (d >= 0.5 * n) d -= n;
    else if (d < -0.5 * n) d += n;
    return d;
}

float tapeAt(const std::vector<float>& ch, std::int64_t len, double p) {
    const auto i0 = std::min((std::int64_t) p, len - 1);
    const auto i1 = i0 + 1 >= len ? 0 : i0 + 1;
    const float fr = (float) (p - (double) i0);
    return ch[(size_t) i0] * (1.0f - fr) + ch[(size_t) i1] * fr;
}

}

StrandStretch::StrandStretch() : play_(kPlaySeed), dub_(kDubSeed) {}

void StrandStretch::prepare(double sampleRate) {
    play_.prepare(sampleRate, 2, kMaxIn);
    dub_.prepare(sampleRate, 2, kChunk);
    const auto preroll = (size_t) std::max(1, play_.prerollSamples());
    for (size_t c = 0; c < 2; ++c) {
        in_[c].assign((size_t) kMaxIn, 0.0f);
        out_[c].assign((size_t) kChunk, 0.0f);
        pre_[c].assign(preroll, 0.0f);
        dubIn_[c].assign((size_t) kChunk, 0.0f);
        dubOut_[c].assign((size_t) kChunk, 0.0f);
    }
    dubLatency_ = (std::int64_t) dub_.inputLatency() + dub_.outputLatency() + kChunk;
    reset();
}

void StrandStretch::reset() {
    play_.reset();
    primed_ = false;
    mix_ = 0.0f;
    warm_ = 0;
    outIdx_ = kChunk;
    inFrac_ = 0.0;
    restartDub();
}

double StrandStretch::leadFor(double ratio) const {
    return step_ * ((double) play_.inputLatency() + (double) play_.outputLatency() * ratio);
}

void StrandStretch::prime(const Tape& tape, std::int64_t len, double pos, int dir, double step,
                          double ratio) {
    dir_ = dir;
    step_ = step;
    feed_ = wrapPos(pos + dir * leadFor(ratio), len);
    const int n = (int) pre_[0].size();
    for (int i = 0; i < n; ++i) {
        const double p = wrapPos(feed_ - dir * step * (double) (n - i), len);
        pre_[0][(size_t) i] = tapeAt(tape[0], len, p);
        pre_[1][(size_t) i] = tapeAt(tape[1], len, p);
    }
    play_.reset();
    const float* pp[2] = { pre_[0].data(), pre_[1].data() };
    play_.preroll(pp, n, ratio);
    outIdx_ = kChunk;
    inFrac_ = 0.0;
    warm_ = (std::int64_t) play_.outputLatency() + kChunk;
    mix_ = 0.0f;
    primed_ = true;
}

void StrandStretch::render(const Tape& tape, std::int64_t len, double pos, double ratio) {
    const double ideal = wrapPos(pos + dir_ * leadFor(ratio), len);
    const double err = wrapDelta(ideal - feed_, len) * dir_ / step_;
    const double base = kChunk * ratio;
    const double pull = std::clamp(err * kPull, -kPullLimit * base, kPullLimit * base);
    const double want = base + pull + inFrac_;
    const int inN = std::clamp((int) want, 0, kMaxIn);
    inFrac_ = std::clamp(want - (double) inN, 0.0, 1.0);
    const double n = (double) len;
    for (int i = 0; i < inN; ++i) {
        in_[0][(size_t) i] = tapeAt(tape[0], len, feed_);
        in_[1][(size_t) i] = tapeAt(tape[1], len, feed_);
        feed_ += dir_ * step_;
        if (feed_ >= n) feed_ -= n;
        else if (feed_ < 0.0) feed_ += n;
        feed_ = wrapPos(feed_, len);
    }
    const float* ip[2] = { in_[0].data(), in_[1].data() };
    float* op[2] = { out_[0].data(), out_[1].data() };
    play_.process(ip, inN, op, kChunk, (double) inN / kChunk);
    outIdx_ = 0;
}

void StrandStretch::play(const Tape& tape, std::int64_t len, double pos, int dir, double step,
                         double ratio, bool engaged, float& l, float& r) {
    if (!available() || len <= 0) return;
    const bool continuous = primed_ && dir == dir_ && step == step_
                            && std::abs(wrapDelta(pos - expect_, len)) < kContinuity;
    expect_ = wrapPos(pos + dir * step * ratio, len);
    if (!continuous) {
        primed_ = false;
        mix_ = 0.0f;
        if (!engaged) return;
        prime(tape, len, pos, dir, step, ratio);
    }
    if (outIdx_ >= kChunk) render(tape, len, pos, ratio);
    const float wl = out_[0][(size_t) outIdx_];
    const float wr = out_[1][(size_t) outIdx_];
    ++outIdx_;
    const float target = engaged && warm_ <= 0 ? 1.0f : 0.0f;
    if (warm_ > 0) --warm_;
    mix_ += std::clamp(target - mix_, -kFade, kFade);
    l += (wl - l) * mix_;
    r += (wr - r) * mix_;
    if (!engaged && mix_ <= 0.0f) primed_ = false;
}

void StrandStretch::restartDub() {
    dub_.reset();
    dubIdx_ = 0;
    for (auto& ch : dubOut_) std::fill(ch.begin(), ch.end(), 0.0f);
}

void StrandStretch::correctDub(float inL, float inR, double transpose, float& outL, float& outR) {
    const auto i = (size_t) dubIdx_;
    dubIn_[0][i] = inL;
    dubIn_[1][i] = inR;
    outL = dubOut_[0][i];
    outR = dubOut_[1][i];
    if (++dubIdx_ < kChunk) return;
    dubIdx_ = 0;
    dub_.setTranspose(transpose);
    const float* ip[2] = { dubIn_[0].data(), dubIn_[1].data() };
    float* op[2] = { dubOut_[0].data(), dubOut_[1].data() };
    dub_.process(ip, kChunk, op, kChunk, 1.0);
}

}
