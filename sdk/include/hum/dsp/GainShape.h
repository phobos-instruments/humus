#pragma once
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "hum/Number.h"

namespace hum {

struct GainShape {
    static constexpr int kMaxPoints = 64;
    int n = 0;
    float px[kMaxPoints] = {};
    float py[kMaxPoints] = {};

    void clear() { n = 0; }
    void add(float x, float y) {
        if (n >= kMaxPoints) return;
        px[n] = std::clamp(x, 0.0f, 1.0f);
        py[n] = std::clamp(y, 0.0f, 1.0f);
        ++n;
    }

    double eval(double phase) const {
        if (n == 0) return 1.0;
        const float x = (float) std::clamp(phase, 0.0, 1.0);
        if (x <= px[0]) return py[0];
        for (int i = 1; i < n; ++i)
            if (x <= px[i]) {
                const float span = px[i] - px[i - 1];
                if (span <= 1e-6f) return py[i];
                const float t = (x - px[i - 1]) / span;
                return py[i - 1] + (py[i] - py[i - 1]) * t;
            }
        return py[n - 1];
    }
};

inline bool decodeGainShape(const char* s, GainShape& out) {
    if (s == nullptr) return false;
    GainShape g;
    const char* p = s;
    while (*p != '\0') {
        while (*p == ' ' || *p == '\n' || *p == '\t') ++p;
        if (*p == '\0') break;
        const char* end = nullptr;
        const double x = scanDouble(p, &end);
        if (end == p || *end != ':') return false;
        p = end + 1;
        const double y = scanDouble(p, &end);
        if (end == p) return false;
        p = end;
        if (g.n >= GainShape::kMaxPoints) return false;
        g.add((float) x, (float) y);
    }
    if (g.n < 2) return false;
    for (int i = 1; i < g.n; ++i)
        if (g.px[i] < g.px[i - 1]) return false;
    out = g;
    return true;
}

inline std::string encodeGainShape(const GainShape& g) {
    std::string s;
    char buf[32];
    for (int i = 0; i < g.n; ++i) {
        std::snprintf(buf, sizeof(buf), "%s%.4g:%.4g", i > 0 ? " " : "",
                      (double) g.px[i], (double) g.py[i]);
        fixDecimalPoint(buf);
        s += buf;
    }
    return s;
}

inline GainShape gainShapePreset(int idx) {
    GainShape g;
    auto duck = [&](float low, float riseEnd, float tail) {
        g.add(0.0f, low);
        g.add(0.08f, low);
        g.add(riseEnd * 0.55f, 0.72f);
        g.add(riseEnd, 1.0f);
        g.add(tail, 1.0f);
        g.add(1.0f, low);
    };
    auto pulses = [&](std::initializer_list<std::pair<float, float>> spans) {
        const float e = 0.012f;
        float cursor = 0.0f;
        for (auto [a, b] : spans) {
            if (a > cursor) { g.add(cursor, 0.0f); g.add(a, 0.0f); }
            g.add(a + e, 1.0f);
            g.add(b - e, 1.0f);
            g.add(b, 0.0f);
            cursor = b;
        }
        if (cursor < 1.0f) g.add(1.0f, 0.0f);
    };
    switch (idx) {
        default:
        case 0: duck(0.02f, 0.38f, 0.96f); break;
        case 1: duck(0.0f, 0.60f, 0.94f); break;
        case 2: duck(0.0f, 0.22f, 0.97f); break;
        case 3:
            g.add(0.0f, 0.0f);
            g.add(0.75f, 1.0f);
            g.add(0.96f, 1.0f);
            g.add(1.0f, 0.0f);
            break;
        case 4:
            g.add(0.0f, 0.0f);
            g.add(0.07f, 0.0f);
            g.add(0.16f, 1.0f);
            g.add(0.97f, 1.0f);
            g.add(1.0f, 0.0f);
            break;
        case 5:
            pulses({{0.02f, 0.16f}, {0.27f, 0.41f}, {0.52f, 0.66f}, {0.77f, 0.91f}});
            break;
        case 6:
            for (int k = 0; k < 4; ++k) {
                const float o = 0.25f * (float) k;
                g.add(o, 0.0f);
                g.add(o + 0.14f, 1.0f);
                g.add(o + 0.23f, 1.0f);
                g.add(o + 0.25f - 0.002f, 0.0f);
            }
            break;
        case 7:
            pulses({{0.06f, 0.27f}, {0.40f, 0.58f}, {0.71f, 0.94f}});
            break;
        case 8:
            g.add(0.0f, 0.0f);
            g.add(1.0f, 1.0f);
            break;
        case 9:
            g.add(0.0f, 0.0f);
            g.add(0.5f, 1.0f);
            g.add(1.0f, 0.0f);
            break;
    }
    return g;
}

inline constexpr int kGainShapePresets = 10;

}
