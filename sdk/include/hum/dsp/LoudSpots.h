// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <vector>

namespace hum {

struct LoudSpot {
    std::int64_t start = 0;
    std::int64_t length = 0;
    float level = 0.0f;
};

struct LoudnessScan {
    float peak = 0.0f;
    std::int64_t peakAt = 0;
    std::vector<LoudSpot> spots;
};

struct LoudnessShape {
    double windowSeconds = 0.4;
    double hopSeconds = 0.1;
    double gapSeconds = 8.0;
    int spots = 3;
};

namespace loudness {

inline std::vector<double> hopEnergy(const float* const* channels, int numChannels,
                                     std::int64_t length, std::int64_t hop, LoudnessScan& scan) {
    std::vector<double> energy((std::size_t) ((length + hop - 1) / hop), 0.0);
    for (std::size_t b = 0; b < energy.size(); ++b) {
        const std::int64_t from = (std::int64_t) b * hop;
        const std::int64_t to = std::min(length, from + hop);
        double sum = 0.0;
        for (int c = 0; c < numChannels; ++c) {
            const float* x = channels[c];
            for (std::int64_t i = from; i < to; ++i) {
                const float a = std::abs(x[i]);
                if (a > scan.peak) {
                    scan.peak = a;
                    scan.peakAt = i;
                }
                sum += (double) x[i] * (double) x[i];
            }
        }
        energy[b] = sum;
    }
    return energy;
}

inline bool farFromAll(const std::vector<std::size_t>& taken, std::size_t at, std::size_t gap) {
    for (const auto t : taken)
        if ((at > t ? at - t : t - at) < gap) return false;
    return true;
}

}

inline LoudnessScan scanLoudness(const float* const* channels, int numChannels,
                                 std::int64_t length, double sampleRate,
                                 const LoudnessShape& shape = {}) {
    LoudnessScan scan;
    if (channels == nullptr || numChannels <= 0 || length <= 0 || sampleRate <= 0.0) return scan;
    const auto hop = std::max<std::int64_t>(1, std::llround(shape.hopSeconds * sampleRate));
    const auto perWindow = (std::size_t) std::max(1L, std::lround(shape.windowSeconds / shape.hopSeconds));
    const auto gap = (std::size_t) std::max(1L, std::lround(shape.gapSeconds / shape.hopSeconds));
    const auto energy = loudness::hopEnergy(channels, numChannels, length, hop, scan);

    const std::size_t windows = energy.size() > perWindow ? energy.size() - perWindow + 1 : 1;
    std::vector<LoudSpot> all(windows);
    double running = 0.0;
    for (std::size_t b = 0; b < std::min(perWindow, energy.size()); ++b) running += energy[b];
    for (std::size_t w = 0; w < windows; ++w) {
        if (w > 0) running += energy[w + perWindow - 1] - energy[w - 1];
        auto& spot = all[w];
        spot.start = (std::int64_t) w * hop;
        spot.length = std::min(length - spot.start, (std::int64_t) perWindow * hop);
        const double frames = (double) spot.length * (double) numChannels;
        spot.level = (float) std::sqrt(std::max(0.0, running) / std::max(1.0, frames));
    }

    std::vector<std::size_t> order(windows);
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&all](std::size_t a, std::size_t b) { return all[a].level > all[b].level; });
    std::vector<std::size_t> taken;
    for (const auto w : order) {
        if ((int) taken.size() >= shape.spots || all[w].level <= 0.0f) break;
        if (loudness::farFromAll(taken, w, gap)) taken.push_back(w);
    }
    for (const auto w : taken) scan.spots.push_back(all[w]);
    return scan;
}

}
