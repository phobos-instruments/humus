// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>

namespace hum::usage {

struct Tally {
    double openSeconds = 0.0;
    double bouncedSeconds = 0.0;
    int bounces = 0;
};

inline Tally afterBounce(Tally t, double seconds) {
    ++t.bounces;
    if (seconds > 0.0 && std::isfinite(seconds)) t.bouncedSeconds += seconds;
    return t;
}

inline std::string spanText(double seconds) {
    const auto minutes = (std::int64_t) std::floor(std::fmax(0.0, seconds) / 60.0);
    if (minutes < 1) return "less than a minute";
    if (minutes < 60) return std::to_string(minutes) + " min";
    const std::int64_t hours = minutes / 60, rest = minutes % 60;
    return std::to_string(hours) + " h" + (rest > 0 ? " " + std::to_string(rest) + " min" : "");
}

}
