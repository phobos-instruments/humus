// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include "hum/ParseInt.h"
#include <cstdint>
#include <string>
#include <vector>

#include "hum/Pattern.h"

namespace hum::rec {

inline std::string nextTakeName(const std::vector<std::string>& existing,
                                const std::string& track, const std::string& ext = ".wav") {
    const std::string prefix = track + "-";
    int maxN = 0;
    for (const auto& f : existing) {
        if (f.rfind(prefix, 0) != 0) continue;
        const auto dot = f.rfind(ext);
        if (dot == std::string::npos || dot <= prefix.size()) continue;
        const std::string digits = f.substr(prefix.size(), dot - prefix.size());
        const int n = parseBoundedInt(digits);
        if (n < 0) continue;
        maxN = std::max(maxN, n);
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%03d", maxN + 1);
    return prefix + buf + ext;
}

struct Lap {
    double startBeat = 0.0;
    std::int64_t offsetSamples = 0;
    std::int64_t lengthSamples = 0;
};

inline std::vector<Lap> splitLaps(double takeStartBeat, std::int64_t totalSamples,
                                  const double* lapBeats, const std::int64_t* lapSamples,
                                  int nLaps) {
    std::vector<Lap> out;
    if (totalSamples <= 0) return out;
    double prevBeat = takeStartBeat;
    std::int64_t prevOff = 0;
    for (int i = 0; i < nLaps; ++i) {
        const std::int64_t end = lapSamples[i];
        if (end > prevOff) out.push_back({prevBeat, prevOff, end - prevOff});
        prevBeat = lapBeats[i];
        prevOff = end;
    }
    if (totalSamples > prevOff) out.push_back({prevBeat, prevOff, totalSamples - prevOff});
    return out;
}

inline int ticksFromSamples(std::int64_t samples, double samplesPerBeat) {
    if (samplesPerBeat <= 0.0) return 0;
    return (int) std::llround((double) samples / samplesPerBeat * Pattern::kTicksPerBeat);
}

}
