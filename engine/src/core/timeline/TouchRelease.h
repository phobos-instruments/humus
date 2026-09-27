// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace hum::touchrelease {

using Key = std::pair<std::string, std::string>;

inline constexpr double kShortestHoldMs = 300.0;
inline constexpr double kLongestHoldMs = 1500.0;
inline constexpr double kMsPerMinute = 60000.0;

inline double holdMs(double bpm) {
    const double beat = bpm > 0.0 ? kMsPerMinute / bpm : kLongestHoldMs;
    return std::clamp(beat, kShortestHoldMs, kLongestHoldMs);
}

inline std::vector<Key> goneQuiet(const std::map<Key, double>& lastTouchMs, double nowMs, double holdForMs) {
    std::vector<Key> out;
    for (const auto& [key, at] : lastTouchMs)
        if (nowMs - at >= holdForMs) out.push_back(key);
    return out;
}

}
