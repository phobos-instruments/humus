// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "hum/Pattern.h"
#include "hum/caps/Params.h"

namespace hum {

inline constexpr int kFullChance = 100;

inline std::vector<int> decodeChanceRow(const std::string& m) {
    std::vector<int> out;
    const char* p = m.c_str();
    while (*p != '\0') {
        char* end = nullptr;
        const long v = std::strtol(p, &end, 10);
        if (end == p) break;
        out.push_back((int) (v < 0 ? 0 : v > kFullChance ? kFullChance : v));
        p = end;
    }
    return out;
}

inline std::string encodeChanceRow(const std::vector<int>& chances) {
    std::string m;
    for (size_t i = 0; i < chances.size(); ++i) {
        if (i > 0) m += ' ';
        m += std::to_string(chances[i]);
    }
    return m;
}

inline int chanceAt(const std::vector<int>& row, long step) {
    return step >= 0 && step < (long) row.size() ? row[(size_t) step] : kFullChance;
}

inline bool allSure(const std::vector<int>& row) {
    for (int c : row)
        if (c < kFullChance) return false;
    return true;
}

inline std::uint32_t stepSeedFor(const std::string& name) {
    std::uint32_t hash = 2166136261u;
    for (unsigned char c : name) {
        hash ^= c;
        hash *= 16777619u;
    }
    return renderSeedIsSet() ? hash ^ renderSeed() : hash;
}

inline bool stepPlays(std::uint32_t seed, int lane, long step, int chance) {
    if (chance >= kFullChance) return true;
    if (chance <= 0) return false;
    std::uint64_t z = ((std::uint64_t) seed << 32) ^ ((std::uint64_t) (std::uint32_t) lane << 24)
                      ^ (std::uint64_t) step;
    z += 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    z ^= z >> 31;
    return (int) (z % (std::uint64_t) kFullChance) < chance;
}

}
