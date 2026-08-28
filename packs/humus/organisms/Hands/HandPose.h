#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "hum/Number.h"

namespace hum {

struct HandLandmarks {
    bool present = false;
    float confidence = 0.0f;
    std::array<std::array<float, 2>, 21> pt{};
};

struct HandValues {
    std::array<float, 5> finger{};
    float x = 0.5f, y = 0.5f;
    float present = 0.0f;
    float pinch = 0.0f;
};

namespace handpose {

inline float dist(const std::array<float, 2>& a, const std::array<float, 2>& b) {
    return std::hypot(a[0] - b[0], a[1] - b[1]);
}

inline float extension(const HandLandmarks& h, int tip, int knuckle,
                       float lo, float hi) {
    const float base = dist(h.pt[(size_t) knuckle], h.pt[0]);
    if (base < 1e-4f) return 0.0f;
    const float r = dist(h.pt[(size_t) tip], h.pt[0]) / base;
    return std::clamp((r - lo) / (hi - lo), 0.0f, 1.0f);
}

inline HandValues values(const HandLandmarks& h) {
    HandValues v;
    if (!h.present) return v;
    v.finger[0] = extension(h, 4, 5, 0.95f, 1.45f);
    v.finger[1] = extension(h, 8, 5, 1.05f, 1.90f);
    v.finger[2] = extension(h, 12, 9, 1.05f, 1.95f);
    v.finger[3] = extension(h, 16, 13, 1.05f, 1.90f);
    v.finger[4] = extension(h, 20, 17, 1.05f, 1.80f);
    v.x = std::clamp(h.pt[0][0], 0.0f, 1.0f);
    v.y = std::clamp(1.0f - h.pt[0][1], 0.0f, 1.0f);
    v.present = 1.0f;
    const float palm = dist(h.pt[9], h.pt[0]);
    if (palm > 1e-4f) {
        const float d = dist(h.pt[4], h.pt[8]) / palm;
        v.pinch = std::clamp(1.0f - (d - 0.15f) / 0.85f, 0.0f, 1.0f);
    }
    return v;
}

constexpr int kGestureSlots = 4;

struct GestureSet {
    std::array<bool, kGestureSlots> learned{};
    std::array<std::array<float, 5>, kGestureSlots> tpl{};
    std::array<int, kGestureSlots> count{};
    std::array<std::array<float, 5>, kGestureSlots> weight{};
};

inline void finalizeWeights(GestureSet& g) {
    for (int k = 0; k < kGestureSlots; ++k) {
        g.weight[(size_t) k].fill(1.0f);
        if (!g.learned[(size_t) k]) continue;
        int nearest = -1;
        float bd = 1e9f;
        for (int j = 0; j < kGestureSlots; ++j) {
            if (j == k || !g.learned[(size_t) j]) continue;
            float d = 0.0f;
            for (int i = 0; i < 5; ++i) {
                const float e = g.tpl[(size_t) k][(size_t) i] - g.tpl[(size_t) j][(size_t) i];
                d += e * e;
            }
            if (d < bd) { bd = d; nearest = j; }
        }
        if (nearest < 0) continue;
        float w[5], sum = 0.0f;
        for (int i = 0; i < 5; ++i) {
            w[i] = 0.25f + std::abs(g.tpl[(size_t) k][(size_t) i]
                                    - g.tpl[(size_t) nearest][(size_t) i]);
            sum += w[i];
        }
        for (int i = 0; i < 5; ++i)
            g.weight[(size_t) k][(size_t) i] = w[i] * 5.0f / std::max(1e-4f, sum);
    }
}

inline void reinforce(GestureSet& g, int slot, const std::array<float, 5>& capture) {
    if (slot < 0 || slot >= kGestureSlots) return;
    if (!g.learned[(size_t) slot]) {
        g.tpl[(size_t) slot] = capture;
        g.learned[(size_t) slot] = true;
        g.count[(size_t) slot] = 1;
        finalizeWeights(g);
        return;
    }
    const int n = std::min(std::max(1, g.count[(size_t) slot]), 31);
    for (int i = 0; i < 5; ++i)
        g.tpl[(size_t) slot][(size_t) i] =
            (g.tpl[(size_t) slot][(size_t) i] * (float) n + capture[(size_t) i])
            / (float) (n + 1);
    g.count[(size_t) slot] = n + 1;
    finalizeWeights(g);
}

inline std::string encodeGestures(const GestureSet& g) {
    std::string s;
    for (int k = 0; k < kGestureSlots; ++k) {
        if (k > 0) s += ";";
        if (!g.learned[(size_t) k]) continue;
        char buf[112];
        const auto& t = g.tpl[(size_t) k];
        std::snprintf(buf, sizeof(buf), "%.3f %.3f %.3f %.3f %.3f %d",
                      (double) t[0], (double) t[1], (double) t[2], (double) t[3],
                      (double) t[4], std::max(1, g.count[(size_t) k]));
        fixDecimalPoint(buf);
        s += buf;
    }
    return s;
}

inline GestureSet decodeGestures(const char* s) {
    GestureSet g;
    if (s == nullptr) return g;
    int slot = 0;
    const char* p = s;
    while (slot < kGestureSlots) {
        int n = 0;
        std::array<float, 6> t{};
        while (n < 6) {
            const char* end = nullptr;
            const double v = scanDouble(p, &end);
            if (end == p) break;
            t[(size_t) n++] = (float) v;
            p = end;
        }
        if (n >= 5) {
            for (int i = 0; i < 5; ++i) g.tpl[(size_t) slot][(size_t) i] = t[(size_t) i];
            g.learned[(size_t) slot] = true;
            g.count[(size_t) slot] = n >= 6 ? std::max(1, (int) t[5]) : 1;
        }
        const char* semi = std::strchr(p, ';');
        if (semi == nullptr) break;
        p = semi + 1;
        ++slot;
    }
    finalizeWeights(g);
    return g;
}

inline float gestureMatch(const std::array<float, 5>& cur,
                          const std::array<float, 5>& tpl, float tolerance,
                          const std::array<float, 5>* weight = nullptr) {
    float acc = 0.0f, wsum = 0.0f;
    for (int i = 0; i < 5; ++i) {
        const float d = cur[(size_t) i] - tpl[(size_t) i];
        const float w = weight != nullptr ? (*weight)[(size_t) i] : 1.0f;
        acc += w * d * d;
        wsum += w;
    }
    const float rms = std::sqrt(acc / std::max(1e-4f, wsum));
    return std::clamp(1.0f - rms / std::max(0.05f, tolerance), 0.0f, 1.0f);
}

inline void matchAll(const GestureSet& g, const std::array<float, 5>& cur,
                     float tolerance, float out[kGestureSlots]) {
    float best = 0.0f;
    for (int k = 0; k < kGestureSlots; ++k) {
        out[k] = g.learned[(size_t) k]
                     ? gestureMatch(cur, g.tpl[(size_t) k], tolerance, &g.weight[(size_t) k])
                     : 0.0f;
        best = std::max(best, out[k]);
    }
    for (int k = 0; k < kGestureSlots; ++k)
        if (out[k] < best)
            out[k] *= std::clamp(1.0f - (best - out[k]) / 0.15f, 0.0f, 1.0f);
}

}
}
