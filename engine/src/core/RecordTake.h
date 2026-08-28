#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "hum/Pattern.h"

namespace hum::rec {

inline std::string nextTakeName(const std::vector<std::string>& existing,
                                const std::string& track) {
    const std::string prefix = track + "-";
    int maxN = 0;
    for (const auto& f : existing) {
        if (f.rfind(prefix, 0) != 0) continue;
        const auto dot = f.rfind(".wav");
        if (dot == std::string::npos || dot <= prefix.size()) continue;
        const std::string digits = f.substr(prefix.size(), dot - prefix.size());
        if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos)
            continue;
        maxN = std::max(maxN, std::stoi(digits));
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%03d", maxN + 1);
    return prefix + buf + ".wav";
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
