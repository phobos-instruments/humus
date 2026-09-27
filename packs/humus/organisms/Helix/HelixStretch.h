// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "common/TimeStretcher.h"

namespace hum {

class StrandStretch {
public:
    using Tape = std::array<std::vector<float>, 2>;
    static constexpr double kMaxSpeedUp = 8.0;

    StrandStretch();

    bool available() const { return play_.available(); }
    bool active() const { return primed_; }
    void prepare(double sampleRate);
    void reset();

    void play(const Tape& tape, std::int64_t len, double pos, int dir, double step, double ratio,
              bool engaged, float& l, float& r);

    std::int64_t dubLatency() const { return dubLatency_; }
    void restartDub();
    void correctDub(float inL, float inR, double transpose, float& outL, float& outR);

    double leadFor(double ratio) const;
    double feedPosition() const { return feed_; }

    static constexpr int kChunk = 64;

private:
    static constexpr int kMaxIn = kChunk * 11;
    static constexpr double kPull = 0.02;
    static constexpr double kPullLimit = 0.25;
    static constexpr float kFade = 1.0f / 256.0f;
    static constexpr double kContinuity = 2.0;
    static_assert(kMaxIn >= kChunk * kMaxSpeedUp * (1.0 + kPullLimit) + 1.0);

    void prime(const Tape& tape, std::int64_t len, double pos, int dir, double step, double ratio);
    void render(const Tape& tape, std::int64_t len, double pos, double ratio);

    TimeStretcher play_, dub_;
    Tape in_, out_, pre_, dubIn_, dubOut_;
    double feed_ = 0.0, expect_ = 0.0, step_ = 1.0, inFrac_ = 0.0;
    int dir_ = 1, outIdx_ = kChunk, dubIdx_ = 0;
    std::int64_t warm_ = 0, dubLatency_ = 0;
    float mix_ = 0.0f;
    bool primed_ = false;
};

}
