// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Helix/Helix.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hum {

void Helix::advance(double& pos, double by, std::int64_t len, bool& wrapped) {
    const double n = (double) len;
    pos += by;
    if (pos >= n) {
        pos = pos < 2.0 * n ? pos - n : std::fmod(pos, n);
        wrapped = true;
    } else if (pos < 0.0) {
        pos = pos >= -n ? pos + n : std::fmod(pos, n) + n;
        if (pos >= n) pos = 0.0;
        wrapped = true;
    }
}

void Helix::holdSlice(Strand& s) {
    if (s.slice <= 0 || s.len <= 0) return;
    const std::int64_t span = s.len / kSlices;
    if (span <= 1) return;
    const std::int64_t from = (std::int64_t) (s.slice - 1) * span;
    const std::int64_t to = s.slice == kSlices ? s.len : from + span;
    if (s.pos >= (double) to) s.pos -= (double) (to - from);
    else if (s.pos < (double) from) s.pos += (double) (to - from);
}

void Helix::runStrand(int t, float inL, float inR, int n, int numSamples, bool frozen,
                      float* const* out, int numOut, float& oL, float& oR) {
    auto& s = strands_[(size_t) t];
    if (s.state == SState::Rec) {
        if (s.recCount < maxLoopSamples_) {
            s.buf[0][(size_t) s.recCount] = inL;
            s.buf[1][(size_t) s.recCount] = inR;
            markPeak(s, s.recCount, inL, inR);
            if (++s.recCount >= maxLoopSamples_) closeLoop(s, 0, true);
        }
        return;
    }
    if ((s.state != SState::Play && s.state != SState::Dub) || frozen) return;

    const auto i0 = (std::int64_t) s.pos;
    const auto i1 = i0 + 1 >= s.len ? 0 : i0 + 1;
    const float fr = (float) (s.pos - (double) i0);
    float l = s.buf[0][(size_t) i0] * (1.0f - fr) + s.buf[0][(size_t) i1] * fr;
    float r = s.buf[1][(size_t) i0] * (1.0f - fr) + s.buf[1][(size_t) i1] * fr;
    auto& st = stretch_[(size_t) t];
    if (s.stretch || st.active())
        st.play(s.buf, s.len, s.pos, s.reverse ? -1 : 1, s.half ? 0.5 : 1.0, s.tempoRatio,
                s.stretch && std::abs(s.tempoRatio - 1.0) > kUnityRatio
                    && s.tempoRatio <= StrandStretch::kMaxSpeedUp,
                l, r);
    if (s.state == SState::Dub) dubSample(s, st, inL, inR);

    const float lvl = s.levelPrev + (s.level - s.levelPrev) * (float) n / (float) numSamples;
    const float dl = l * lvl, dr = r * lvl;
    oL += dl;
    oR += dr;
    const int d0 = 2 + t * 2;
    if (numOut > d0) out[d0][n] = dl;
    if (numOut > d0 + 1) out[d0 + 1][n] = dr;

    bool wrapped = false;
    const double bent = s.speed * (1.0 + kNudgeBend * (double) s.nudge);
    advance(s.pos, s.reverse ? -bent : bent, s.len, wrapped);
    holdSlice(s);
    if (wrapped && s.oneShot && s.state == SState::Play) {
        s.state = SState::Stopped;
        s.pos = 0.0;
    }
}

void Helix::dubSample(Strand& s, StrandStretch& st, float inL, float inR) {
    if (s.dubLag <= 0) {
        writeDub(s, (std::int64_t) s.pos, inL, inR);
        return;
    }
    if (s.dubFresh) {
        st.restartDub();
        s.dubFresh = false;
    }
    float wl = 0.0f, wr = 0.0f;
    st.correctDub(inL, inR, s.tempoRatio, wl, wr);
    if (s.dubWarm > 0) {
        --s.dubWarm;
    } else {
        writeDub(s, (std::int64_t) s.dubHead, wl, wr);
        bool wrapped = false;
        advance(s.dubHead, s.reverse ? -s.speed : s.speed, s.len, wrapped);
    }
    if (s.dubTail >= 0 && --s.dubTail < 0) leaveDub(s);
}

void Helix::writeDub(Strand& s, std::int64_t index, float l, float r) {
    if (s.len <= 0 || index == s.lastWrite) return;
    index = std::clamp<std::int64_t>(index, 0, s.len - 1);
    std::int64_t steps = 1;
    if (s.lastWrite >= 0) {
        const auto gap = (((index - s.lastWrite) * s.dubDir) % s.len + s.len) % s.len;
        if (gap > 1 && gap <= kMaxFill) steps = gap;
    }
    for (std::int64_t k = steps - 1; k >= 0; --k) {
        const auto j = (size_t) (((index - s.dubDir * k) % s.len + s.len) % s.len);
        snapshotAhead(s, s.dubWritten + 1);
        s.buf[0][j] = (float) (s.buf[0][j] * decay_) + l;
        s.buf[1][j] = (float) (s.buf[1][j] * decay_) + r;
        markPeak(s, (std::int64_t) j, s.buf[0][j], s.buf[1][j]);
        ++s.dubWritten;
    }
    s.lastWrite = index;
}

}
