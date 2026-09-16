// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "hum/Number.h"

namespace hum {
namespace gvec {

constexpr int kSlots = 4;
constexpr int kMaxDims = 8;

struct Set {
    int dims = 5;
    std::array<bool, kSlots> learned{};
    std::array<std::array<float, kMaxDims>, kSlots> tpl{};
    std::array<int, kSlots> count{};
    std::array<std::array<float, kMaxDims>, kSlots> weight{};
};

inline void finalizeWeights(Set& g) {
    const int n = std::clamp(g.dims, 1, kMaxDims);
    for (int k = 0; k < kSlots; ++k) {
        g.weight[(size_t) k].fill(1.0f);
        if (!g.learned[(size_t) k]) continue;
        int nearest = -1;
        float bd = 1e9f;
        for (int j = 0; j < kSlots; ++j) {
            if (j == k || !g.learned[(size_t) j]) continue;
            float d = 0.0f;
            for (int i = 0; i < n; ++i) {
                const float e = g.tpl[(size_t) k][(size_t) i] - g.tpl[(size_t) j][(size_t) i];
                d += e * e;
            }
            if (d < bd) { bd = d; nearest = j; }
        }
        if (nearest < 0) continue;
        float w[kMaxDims], sum = 0.0f;
        for (int i = 0; i < n; ++i) {
            w[i] = 0.25f + std::abs(g.tpl[(size_t) k][(size_t) i]
                                    - g.tpl[(size_t) nearest][(size_t) i]);
            sum += w[i];
        }
        for (int i = 0; i < n; ++i)
            g.weight[(size_t) k][(size_t) i] = w[i] * (float) n / std::max(1e-4f, sum);
    }
}

inline void reinforce(Set& g, int slot, const float* capture) {
    if (slot < 0 || slot >= kSlots) return;
    const int dims = std::clamp(g.dims, 1, kMaxDims);
    if (!g.learned[(size_t) slot]) {
        for (int i = 0; i < dims; ++i) g.tpl[(size_t) slot][(size_t) i] = capture[i];
        g.learned[(size_t) slot] = true;
        g.count[(size_t) slot] = 1;
        finalizeWeights(g);
        return;
    }
    const int n = std::min(std::max(1, g.count[(size_t) slot]), 31);
    for (int i = 0; i < dims; ++i)
        g.tpl[(size_t) slot][(size_t) i] =
            (g.tpl[(size_t) slot][(size_t) i] * (float) n + capture[i]) / (float) (n + 1);
    g.count[(size_t) slot] = n + 1;
    finalizeWeights(g);
}

inline std::string encode(const Set& g) {
    const int dims = std::clamp(g.dims, 1, kMaxDims);
    std::string s;
    for (int k = 0; k < kSlots; ++k) {
        if (k > 0) s += ";";
        if (!g.learned[(size_t) k]) continue;
        for (int i = 0; i < dims; ++i) {
            char buf[24];
            std::snprintf(buf, sizeof(buf), "%.3f ", (double) g.tpl[(size_t) k][(size_t) i]);
            fixDecimalPoint(buf);
            s += buf;
        }
        s += std::to_string(std::max(1, g.count[(size_t) k]));
    }
    return s;
}

inline Set decode(const char* s, int dims) {
    Set g;
    g.dims = std::clamp(dims, 1, kMaxDims);
    if (s == nullptr) return g;
    int slot = 0;
    const char* p = s;
    while (slot < kSlots) {
        int n = 0;
        std::array<float, kMaxDims + 1> t{};
        while (n < g.dims + 1) {
            const char* end = nullptr;
            const double v = scanDouble(p, &end);
            if (end == p) break;
            t[(size_t) n++] = (float) v;
            p = end;
        }
        if (n >= g.dims) {
            for (int i = 0; i < g.dims; ++i) g.tpl[(size_t) slot][(size_t) i] = t[(size_t) i];
            g.learned[(size_t) slot] = true;
            g.count[(size_t) slot] = n > g.dims ? std::max(1, (int) t[(size_t) g.dims]) : 1;
        }
        const char* semi = std::strchr(p, ';');
        if (semi == nullptr) break;
        p = semi + 1;
        ++slot;
    }
    finalizeWeights(g);
    return g;
}

inline float match(const float* cur, const float* tpl, int dims, float tolerance,
                   const float* weight = nullptr) {
    float acc = 0.0f, wsum = 0.0f;
    for (int i = 0; i < dims; ++i) {
        const float d = cur[i] - tpl[i];
        const float w = weight != nullptr ? weight[i] : 1.0f;
        acc += w * d * d;
        wsum += w;
    }
    const float rms = std::sqrt(acc / std::max(1e-4f, wsum));
    return std::clamp(1.0f - rms / std::max(0.05f, tolerance), 0.0f, 1.0f);
}

inline void matchAll(const Set& g, const float* cur, float tolerance, float out[kSlots]) {
    const int dims = std::clamp(g.dims, 1, kMaxDims);
    float best = 0.0f;
    for (int k = 0; k < kSlots; ++k) {
        out[k] = g.learned[(size_t) k]
                     ? match(cur, g.tpl[(size_t) k].data(), dims, tolerance,
                             g.weight[(size_t) k].data())
                     : 0.0f;
        best = std::max(best, out[k]);
    }
    for (int k = 0; k < kSlots; ++k)
        if (out[k] < best)
            out[k] *= std::clamp(1.0f - (best - out[k]) / 0.15f, 0.0f, 1.0f);
}

}
}
