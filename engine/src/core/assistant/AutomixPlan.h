// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace hum {
namespace automix {

struct Source {
    std::string name;
    bool anchor = false;
    double peak = 0.0;
    double value = 0.0;
    double min = 0.0, max = 1.0;
};

struct Move {
    std::string name;
    double from = 0.0, to = 0.0;
    bool clamped = false;
};

constexpr double kAnchorTarget = 0.5;
constexpr double kOtherTarget = 0.35;
constexpr double kMasterTarget = 0.6;
constexpr double kSilentPeak = 1.0e-3;

inline size_t pickAnchor(const std::vector<Source>& sources) {
    size_t loudest = (size_t) -1;
    for (size_t i = 0; i < sources.size(); ++i) {
        if (sources[i].peak < kSilentPeak) continue;
        if (sources[i].anchor) return i;
        if (loudest == (size_t) -1 || sources[i].peak > sources[loudest].peak) loudest = i;
    }
    return loudest;
}

inline std::vector<Move> plan(const std::vector<Source>& sources, size_t anchor,
                              double anchorTarget = kAnchorTarget,
                              double otherTarget = kOtherTarget) {
    std::vector<Move> moves;
    for (size_t i = 0; i < sources.size(); ++i) {
        const auto& s = sources[i];
        if (s.peak < kSilentPeak || s.value <= 0.0) continue;
        const double target = i == anchor ? anchorTarget : otherTarget;
        const double ideal = s.value * (target / s.peak);
        const double to = std::fmin(s.max, std::fmax(s.min, ideal));
        moves.push_back({s.name, s.value, to, to != ideal});
    }
    return moves;
}

}
}
