#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace hum {

struct SliceOnset {
    int pos = 0;
    float strength = 0.0f;
};

inline constexpr int kMaxSliceOnsets = 64;

inline constexpr int kOnsetHop = 256;

inline int refineOnset(const float* mono, int from, int to) {
    float localPeak = 0.0f;
    for (int i = from; i < to; ++i) localPeak = std::max(localPeak, std::abs(mono[i]));
    for (int i = from; i < to; ++i)
        if (std::abs(mono[i]) >= localPeak * 0.5f) return i;
    return from;
}

template <typename Refine>
inline std::vector<SliceOnset> pickOnsets(const double* hopEnergy, int hops, double sampleRate,
                                          double minGapSeconds, Refine refine) {
    std::vector<SliceOnset> out;
    out.push_back({0, 1.0f});
    if (hopEnergy == nullptr || hops < 3 || sampleRate <= 0.0) return out;
    const int hop = kOnsetHop;
    std::vector<float> flux((size_t) hops, 0.0f);
    double prevLog = 0.0;
    for (int k = 0; k < hops; ++k) {
        const double lg = std::log10(hopEnergy[k] + 1e-9);
        if (k > 0) flux[(size_t) k] = (float) std::max(0.0, lg - prevLog);
        prevLog = lg;
    }
    const int minGapHops = std::max(1, (int) (minGapSeconds * sampleRate / hop));
    float maxFlux = 0.0f;
    for (float f : flux) maxFlux = std::max(maxFlux, f);
    if (maxFlux <= 0.0f) return out;

    for (int k = 1; k < hops - 1; ++k) {
        const float f = flux[(size_t) k];
        if (f < maxFlux * 0.02f) continue;
        if (f < flux[(size_t) (k - 1)] || f <= flux[(size_t) (k + 1)]) continue;
        const int pos = refine(k);
        const float strength = std::min(0.995f, f / maxFlux);
        if (pos < minGapHops * hop) continue;
        if (!out.empty() && pos - out.back().pos < minGapHops * hop) {
            if (strength > out.back().strength && out.back().pos != 0)
                out.back() = {pos, strength};
            continue;
        }
        out.push_back({pos, strength});
    }
    return out;
}

inline std::vector<SliceOnset> detectSliceOnsets(const float* mono, int n,
                                                 double sampleRate,
                                                 double minGapSeconds = 0.05) {
    if (mono == nullptr || n <= 0) return {};
    const int hop = kOnsetHop;
    const int hops = n / hop;
    std::vector<double> energy((size_t) std::max(0, hops), 0.0);
    for (int k = 0; k < hops; ++k) {
        double acc = 0.0;
        const float* h = mono + (size_t) k * hop;
        for (int i = 0; i < hop; ++i) acc += (double) h[i] * h[i];
        energy[(size_t) k] = acc / hop;
    }
    auto out = pickOnsets(energy.data(), hops, sampleRate, minGapSeconds, [&](int k) {
        return refineOnset(mono, std::max(0, (k - 1) * hop), std::min(n, (k + 1) * hop));
    });

    if ((int) out.size() > kMaxSliceOnsets) {
        std::vector<SliceOnset> sorted(out.begin() + 1, out.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const SliceOnset& a, const SliceOnset& b) { return a.strength > b.strength; });
        sorted.resize((size_t) kMaxSliceOnsets - 1);
        std::sort(sorted.begin(), sorted.end(),
                  [](const SliceOnset& a, const SliceOnset& b) { return a.pos < b.pos; });
        out.resize(1);
        out.insert(out.end(), sorted.begin(), sorted.end());
    }
    return out;
}

inline void activeSliceOnsets(const std::vector<SliceOnset>& all, double sense,
                              std::vector<SliceOnset>& out) {
    out.clear();
    const float thresh = (float) (1.0 - std::min(1.0, std::max(0.0, sense)));
    for (const auto& o : all)
        if (o.strength >= thresh || o.pos == 0) out.push_back(o);
}

inline void slicePermutation(int count, int seed, int* perm) {
    for (int i = 0; i < count; ++i) perm[i] = i;
    if (seed == 0 || count < 2) return;
    std::uint32_t s = (std::uint32_t) seed * 0x9e3779b9u + 1u;
    for (int i = count - 1; i > 0; --i) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        std::swap(perm[i], perm[s % (std::uint32_t) (i + 1)]);
    }
}

}
