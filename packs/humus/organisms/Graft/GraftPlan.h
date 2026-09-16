// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/dsp/DspMath.h"

namespace hum {

enum class GraftOrder { Random, InOrder, RoundRobin, Blocks, Mirror, One };
enum class GraftCut { Free, Grid, Onsets };

inline constexpr int kGraftSlots = 8;
inline constexpr int kGraftMaxSlices = 64;
inline constexpr double kGraftHeadTailSeconds = 0.002;
inline constexpr double kGraftSnapFreeSeconds = 0.010;
inline constexpr double kGraftSnapGridSeconds = 0.002;
inline constexpr double kGraftSnapOnsetSeconds = 0.004;
inline constexpr double kGraftSilenceSeconds = 0.02;
inline constexpr double kGraftMaxSeconds = 120.0;
inline constexpr double kGraftMinStretch = 0.5;
inline constexpr double kGraftMaxStretch = 2.0;
inline constexpr int kGraftMinFragment = 64;

struct GraftSource {
    juce::AudioBuffer<float> buf;
    double rate = kDefaultSampleRate;
    std::vector<float> mono;
    std::vector<int> cuts;

    bool ready() const { return buf.getNumSamples() > 1 && rate > 0.0; }
    int frames() const { return buf.getNumSamples(); }
};

struct GraftSlot {
    int source = -1;
    int fragment = 0;
    bool reverse = false;
    int pitch = 0;
    int gainPct = 100;
};

struct GraftSpec {
    int slices = 16;
    GraftOrder order = GraftOrder::Random;
    GraftCut cut = GraftCut::Free;
    double gridSeconds = 0.25;
    double lengthPct = 100.0;
    double xfadeMs = 6.0;
    bool normalise = true;
    int seed = 0;
    int one = 0;
    int bars = 0;
    double barSeconds = 2.0;
};

inline bool graftCutsAtOnsets(const GraftSource& s, const GraftSpec& spec) {
    return spec.cut == GraftCut::Onsets && s.cuts.size() > 1;
}

inline int graftFragments(const GraftSource& s, const GraftSpec& spec) {
    if (!s.ready()) return 1;
    if (graftCutsAtOnsets(s, spec)) return (int) s.cuts.size();
    if (spec.cut != GraftCut::Grid) return std::clamp(spec.slices, 1, kGraftMaxSlices);
    const int grid = std::max(kGraftMinFragment, (int) std::lround(spec.gridSeconds * s.rate));
    return std::max(1, s.frames() / grid);
}

inline void graftRegion(const GraftSource& s, const GraftSpec& spec, int fragment,
                        int& from, int& to) {
    const int count = graftFragments(s, spec);
    const int k = ((fragment % count) + count) % count;
    if (graftCutsAtOnsets(s, spec)) {
        from = s.cuts[(size_t) k];
        to = k + 1 < (int) s.cuts.size() ? s.cuts[(size_t) k + 1] : s.frames();
        return;
    }
    if (spec.cut == GraftCut::Grid) {
        const int grid = std::max(kGraftMinFragment, (int) std::lround(spec.gridSeconds * s.rate));
        from = k * grid;
        to = std::min(from + grid, s.frames());
        return;
    }
    const int step = std::max(1, s.frames() / std::clamp(spec.slices, 1, kGraftMaxSlices));
    from = k * step;
    const int want = std::max(kGraftMinFragment,
                              (int) std::lround(step * spec.lengthPct / 100.0));
    to = std::min(s.frames(), from + want);
}

inline int graftSnap(const GraftSource& s, int at, int window) {
    const int n = s.frames();
    if (at <= 0 || at >= n || window < 2 || (int) s.mono.size() < n) return std::clamp(at, 0, n);
    const int lo = std::max(1, at - window), hi = std::min(n - 1, at + window);
    int rising = -1, either = -1, risingDist = window + 1, eitherDist = window + 1;
    for (int i = lo; i < hi; ++i) {
        const float a = s.mono[(size_t) i - 1], b = s.mono[(size_t) i];
        const bool up = a <= 0.0f && b > 0.0f;
        if (!up && !(a >= 0.0f && b < 0.0f)) continue;
        const int d = std::abs(i - at);
        if (up && d < risingDist) { risingDist = d; rising = i; }
        if (d < eitherDist) { eitherDist = d; either = i; }
    }
    if (rising >= 0) return rising;
    return either >= 0 ? either : at;
}

inline int graftSnapWindow(const GraftSource& s, const GraftSpec& spec) {
    const double seconds = spec.cut == GraftCut::Grid     ? kGraftSnapGridSeconds
                           : spec.cut == GraftCut::Onsets ? kGraftSnapOnsetSeconds
                                                          : kGraftSnapFreeSeconds;
    return (int) std::lround(seconds * s.rate);
}

inline std::vector<int> graftLoaded(const std::vector<GraftSource>& sources) {
    std::vector<int> out;
    for (int i = 0; i < (int) sources.size(); ++i)
        if (sources[(size_t) i].ready()) out.push_back(i);
    return out;
}

inline void graftPlan(const std::vector<GraftSource>& sources, const GraftSpec& spec,
                      const std::map<int, GraftSlot>& pins, std::vector<GraftSlot>& out) {
    const int n = std::clamp(spec.slices, 1, kGraftMaxSlices);
    out.assign((size_t) n, GraftSlot{});
    const std::vector<int> list = graftLoaded(sources);
    const int ns = (int) list.size();
    if (ns == 0) return;

    std::uint32_t state = (std::uint32_t) spec.seed * 0x9e3779b9u + 1u;
    auto roll = [&state] {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    auto pick = [&](int source) { return graftFragments(sources[(size_t) source], spec); };
    const int walk = std::max(0, spec.seed);

    for (int i = 0; i < n; ++i) {
        GraftSlot& slot = out[(size_t) i];
        if (const auto it = pins.find(i); it != pins.end()) {
            const int pinned = it->second.source;
            if (pinned >= 0 && pinned < (int) sources.size() && sources[(size_t) pinned].ready()) {
                slot = it->second;
                continue;
            }
        }
        switch (spec.order) {
            case GraftOrder::Random:
                slot.source = list[(size_t) (roll() % (std::uint32_t) ns)];
                slot.fragment = (int) (roll() % (std::uint32_t) pick(slot.source));
                break;
            case GraftOrder::InOrder:
                slot.source = list[(size_t) ((i + walk) % ns)];
                slot.fragment = (i + walk) % pick(slot.source);
                break;
            case GraftOrder::RoundRobin:
                slot.source = list[(size_t) ((i + walk) % ns)];
                slot.fragment = (i / ns + walk) % pick(slot.source);
                break;
            case GraftOrder::Blocks: {
                const int block = std::min(ns - 1, i * ns / n);
                slot.source = list[(size_t) ((block + walk) % ns)];
                const int first = (block * n + ns - 1) / ns;
                const int count = pick(slot.source);
                slot.fragment = ((i - first + walk) % count + count) % count;
                break;
            }
            case GraftOrder::Mirror: {
                const int half = (n + 1) / 2;
                if (i < half) {
                    slot.source = list[(size_t) (roll() % (std::uint32_t) ns)];
                    slot.fragment = (int) (roll() % (std::uint32_t) pick(slot.source));
                } else {
                    const GraftSlot& twin = out[(size_t) (n - 1 - i)];
                    slot.source = twin.source;
                    slot.fragment = twin.fragment;
                    slot.reverse = !twin.reverse;
                }
                break;
            }
            case GraftOrder::One: {
                const bool usable = spec.one >= 0 && spec.one < (int) sources.size()
                                    && sources[(size_t) spec.one].ready();
                slot.source = usable ? spec.one : list[0];
                slot.fragment = (i + walk) % pick(slot.source);
                break;
            }
        }
    }
}

}
