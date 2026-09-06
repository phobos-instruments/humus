#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "common/GestureVec.h"
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

constexpr int kGestureSlots = gvec::kSlots;

using GestureSet = gvec::Set;

inline void reinforce(GestureSet& g, int slot, const std::array<float, 5>& capture) {
    gvec::reinforce(g, slot, capture.data());
}

inline std::string encodeGestures(const GestureSet& g) { return gvec::encode(g); }

inline GestureSet decodeGestures(const char* s) { return gvec::decode(s, 5); }

inline float gestureMatch(const std::array<float, 5>& cur,
                          const std::array<float, gvec::kMaxDims>& tpl, float tolerance,
                          const std::array<float, gvec::kMaxDims>* weight = nullptr) {
    return gvec::match(cur.data(), tpl.data(), 5, tolerance,
                       weight != nullptr ? weight->data() : nullptr);
}

inline void matchAll(const GestureSet& g, const std::array<float, 5>& cur,
                     float tolerance, float out[kGestureSlots]) {
    gvec::matchAll(g, cur.data(), tolerance, out);
}

}
}
