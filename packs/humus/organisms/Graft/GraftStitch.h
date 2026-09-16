// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "Graft/GraftPlan.h"

#include "hum/dsp/DspMath.h"
#include "hum/dsp/FadeLaw.h"
#include "hum/dsp/Interpolation.h"

namespace hum {

struct GraftResult {
    juce::AudioBuffer<float> buf;
    std::vector<float> bounds;
    double stretch = 1.0;
    bool capped = false;
};

namespace detail {

struct GraftPiece {
    int source = -1;
    int from = 0;
    int span = 0;
    int length = 0;
    bool reverse = false;
    float gain = 1.0f;
};

inline void graftMeasure(const std::vector<GraftSource>& sources,
                         const std::vector<GraftSlot>& slots, const GraftSpec& spec,
                         double destRate, std::vector<GraftPiece>& out) {
    const int silence = std::max(1, (int) std::lround(kGraftSilenceSeconds * destRate));
    out.clear();
    out.reserve(slots.size());
    for (const auto& slot : slots) {
        GraftPiece piece;
        const bool usable = slot.source >= 0 && slot.source < (int) sources.size()
                            && sources[(size_t) slot.source].ready();
        if (!usable) {
            piece.span = piece.length = silence;
            out.push_back(piece);
            continue;
        }
        const GraftSource& s = sources[(size_t) slot.source];
        int from = 0, to = 0;
        graftRegion(s, spec, slot.fragment, from, to);
        const int window = graftSnapWindow(s, spec);
        from = graftSnap(s, from, window);
        to = graftSnap(s, to, window);
        if (to - from < kGraftMinFragment) to = std::min(s.frames(), from + kGraftMinFragment);
        if (to <= from) {
            piece.span = piece.length = silence;
            out.push_back(piece);
            continue;
        }
        const double rise = std::pow(2.0, slot.pitch / kSemitonesPerOctave);
        piece.source = slot.source;
        piece.from = from;
        piece.span = to - from;
        piece.length = std::max(16, (int) std::lround(piece.span * destRate / (s.rate * rise)));
        piece.reverse = slot.reverse;
        piece.gain = (float) slot.gainPct / 100.0f;
        out.push_back(piece);
    }
}

inline int graftLayout(const std::vector<GraftPiece>& pieces, int overlapFrames,
                       std::vector<int>& overlaps, std::vector<int>& starts) {
    const int n = (int) pieces.size();
    overlaps.assign((size_t) n + 1, 0);
    starts.assign((size_t) n, 0);
    for (int i = 1; i < n; ++i)
        overlaps[(size_t) i] = std::max(0, std::min({overlapFrames,
                                                     pieces[(size_t) i - 1].length / 3,
                                                     pieces[(size_t) i].length / 3}));
    int at = 0;
    for (int i = 0; i < n; ++i) {
        if (i > 0) at -= overlaps[(size_t) i];
        starts[(size_t) i] = at;
        at += pieces[(size_t) i].length;
    }
    return at;
}

}

inline GraftResult graftStitch(const std::vector<GraftSource>& sources,
                               const std::vector<GraftSlot>& slots, const GraftSpec& spec,
                               double destRate) {
    GraftResult result;
    result.buf.setSize(2, 0);
    if (slots.empty() || destRate <= 0.0 || graftLoaded(sources).empty()) return result;

    std::vector<detail::GraftPiece> pieces;
    detail::graftMeasure(sources, slots, spec, destRate, pieces);

    const int overlapFrames = std::max(0, (int) std::lround(spec.xfadeMs * 0.001 * destRate));
    std::vector<int> overlaps, starts;
    int total = detail::graftLayout(pieces, overlapFrames, overlaps, starts);

    if (spec.bars > 0 && spec.barSeconds > 0.0 && total > 0) {
        const int wanted = (int) std::lround(spec.bars * spec.barSeconds * destRate);
        result.stretch = std::clamp((double) wanted / (double) total,
                                    kGraftMinStretch, kGraftMaxStretch);
        for (auto& piece : pieces)
            piece.length = std::max(16, (int) std::lround(piece.length * result.stretch));
        total = detail::graftLayout(pieces, overlapFrames, overlaps, starts);
        const int drift = wanted - total;
        auto& last = pieces.back();
        if (drift != 0 && last.length + drift >= 16) {
            last.length += drift;
            total = detail::graftLayout(pieces, overlapFrames, overlaps, starts);
        }
    }

    const int cap = (int) std::lround(kGraftMaxSeconds * destRate);
    if (total > cap) { total = cap; result.capped = true; }
    if (total < kGraftMinFragment) return result;

    result.buf.setSize(2, total);
    result.buf.clear();
    const int headTail = std::max(4, (int) std::lround(kGraftHeadTailSeconds * destRate));
    const int n = (int) pieces.size();

    for (int i = 0; i < n; ++i) {
        const auto& piece = pieces[(size_t) i];
        const int at = starts[(size_t) i];
        if (at >= total) break;
        const int length = std::min(piece.length, total - at);
        if (length < 1 || piece.source < 0) continue;

        const GraftSource& s = sources[(size_t) piece.source];
        const double step = (double) piece.span / (double) piece.length;
        const int fadeIn = i == 0 ? std::min(headTail, length / 3)
                                  : std::min(overlaps[(size_t) i], length);
        const int fadeOut = i == n - 1 ? std::min(headTail, length / 3)
                                       : std::min(overlaps[(size_t) i + 1], length);

        for (int c = 0; c < 2; ++c) {
            const float* in = s.buf.getReadPointer(std::min(c, s.buf.getNumChannels() - 1));
            float* dst = result.buf.getWritePointer(c);
            for (int j = 0; j < length; ++j) {
                const double offset = piece.reverse ? (double) (length - 1 - j) : (double) j;
                float v = sampleAt(in, s.frames(), piece.from + offset * step, Interp::Sinc);
                if (fadeIn > 1 && j < fadeIn)
                    v *= fadeGain((float) j / (float) (fadeIn - 1), kFadeEqualPower);
                if (fadeOut > 1 && j >= length - fadeOut)
                    v *= fadeGain((float) (length - 1 - j) / (float) (fadeOut - 1),
                                  kFadeEqualPower);
                dst[at + j] += v * piece.gain;
            }
        }
    }

    if (spec.normalise) {
        const float peak = result.buf.getMagnitude(0, total);
        if (peak > 1.0e-6f) result.buf.applyGain(0.98f / peak);
    }

    result.bounds.reserve(slots.size() + 1);
    for (int i = 0; i < n; ++i)
        result.bounds.push_back(std::min(1.0f, (float) starts[(size_t) i] / (float) total));
    result.bounds.push_back(1.0f);
    return result;
}

}
