#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "hum/Number.h"

namespace hum {

inline constexpr int kWaveTableLen = 1024;

inline float waveTableRead(const float* t, int len, double phase01) {
    const double x = (phase01 - std::floor(phase01)) * (double) len;
    const int i0 = (int) x;
    const float f = (float) (x - (double) i0);
    const int m = len - 1;
    const float xm1 = t[(i0 - 1) & m], x0 = t[i0 & m];
    const float x1 = t[(i0 + 1) & m], x2 = t[(i0 + 2) & m];
    const float a = -0.5f * xm1 + 1.5f * x0 - 1.5f * x1 + 0.5f * x2;
    const float b = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c = -0.5f * xm1 + 0.5f * x1;
    return ((a * f + b) * f + c) * f + x0;
}

inline void waveTableResample(const float* x, int n, float* out, int len) {
    if (n <= 0 || len <= 0) return;
    for (int i = 0; i < len; ++i) {
        const double pos = (double) i * (double) n / (double) len;
        const int j = (int) pos;
        const float f = (float) (pos - (double) j);
        const float a = x[j % n], b = x[(j + 1) % n];
        out[i] = a + (b - a) * f;
    }
}

inline void waveTableTame(float* t, int len) {
    double mean = 0.0;
    for (int i = 0; i < len; ++i) mean += t[i];
    mean /= (double) std::max(1, len);
    double peak = 0.0;
    for (int i = 0; i < len; ++i) {
        t[i] = (float) (t[i] - mean);
        peak = std::max(peak, (double) std::abs(t[i]));
    }
    const float g = peak > 1e-9 ? (float) (0.9 / peak) : 0.0f;
    for (int i = 0; i < len; ++i) t[i] = std::clamp(t[i] * g, -1.0f, 1.0f);
}

inline bool decodeWaveTable(const char* s, float* out, int len) {
    if (s == nullptr) return false;
    constexpr int kMax = 4096;
    float buf[kMax];
    int n = 0;
    const char* p = s;
    while (*p != '\0') {
        while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r') ++p;
        if (*p == '\0') break;
        const char* end = nullptr;
        const double v = scanDouble(p, &end);
        if (end == p || n >= kMax) return false;
        buf[n++] = std::clamp((float) v, -1.0f, 1.0f);
        p = end;
    }
    if (n < 4) return false;
    if (n == len) std::copy(buf, buf + n, out);
    else waveTableResample(buf, n, out, len);
    return true;
}

inline std::string encodeWaveTable(const float* t, int len) {
    std::string s;
    s.reserve((size_t) len * 8);
    char buf[32];
    for (int i = 0; i < len; ++i) {
        std::snprintf(buf, sizeof(buf), "%s%.4g", i > 0 ? " " : "", (double) t[i]);
        fixDecimalPoint(buf);
        s += buf;
    }
    return s;
}

inline void waveTablePreset(int idx, float* out, int len) {
    for (int i = 0; i < len; ++i) {
        const double ph = (double) i / (double) len;
        switch (idx) {
            default:
            case 0: out[i] = (float) std::sin(2.0 * 3.14159265358979323846 * ph); break;
            case 1: out[i] = (float) (ph < 0.5 ? 4.0 * ph - 1.0 : 3.0 - 4.0 * ph); break;
            case 2: out[i] = (float) (2.0 * ph - 1.0); break;
            case 3: out[i] = ph < 0.5 ? 1.0f : -1.0f; break;
        }
    }
    if (idx == 3) out[0] = 0.0f;
}
inline constexpr int kWaveTablePresets = 4;

inline void waveTableRandom(uint32_t seed, float* out, int len) {
    if (seed == 0) seed = 0x9e3779b9u;
    auto rnd = [&seed]() {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return seed;
    };
    auto uni = [&]() { return (double) (rnd() & 0xFFFFFF) / (double) 0xFFFFFF; };

    const int partials = 3 + (int) (uni() * uni() * 29.0);
    const double tilt = 0.6 + uni() * 1.2;
    const int stride = uni() < 0.25 ? 2 : 1;
    const double formant = 2.0 + uni() * 12.0;
    const double formantW = 1.0 + uni() * 4.0;
    const double formantG = uni() * 3.0;
    const bool fold = uni() < 0.35;
    const double drive = 1.5 + uni() * 2.5;

    for (int i = 0; i < len; ++i) out[i] = 0.0f;
    for (int k = 1, count = 0; count < partials; k += stride, ++count) {
        const double lobe = (double) k - formant;
        double a = (0.35 + 0.65 * uni()) / std::pow((double) k, tilt);
        a *= 1.0 + formantG * std::exp(-(lobe * lobe) / (2.0 * formantW * formantW));
        const double ph = uni();
        for (int i = 0; i < len; ++i)
            out[i] += (float) (a * std::sin(2.0 * 3.14159265358979323846
                                            * ((double) k * i / (double) len + ph)));
    }
    if (fold)
        for (int i = 0; i < len; ++i) out[i] = (float) std::tanh(drive * out[i]);
    waveTableTame(out, len);
}

inline void waveTableFromCycle(const float* x, int n, float* out, int len) {
    waveTableResample(x, n, out, len);
    waveTableTame(out, len);
}

inline void waveTableFromBytes(const uint8_t* data, int n, float* out, int len) {
    if (data == nullptr || n <= 0) {
        std::fill(out, out + len, 0.0f);
        return;
    }
    constexpr int kMax = 4096;
    float buf[kMax];
    const int m = std::min(n, kMax);
    for (int i = 0; i < m; ++i) {
        const int src = (int) ((int64_t) i * n / m);
        buf[i] = (float) data[src] / 127.5f - 1.0f;
    }
    waveTableResample(buf, m, out, len);
    waveTableTame(out, len);
}

inline int waveTableEstimatePeriod(const float* x, int n, int minP, int maxP) {
    minP = std::max(2, minP);
    maxP = std::min(maxP, n / 2);
    if (maxP <= minP) return 0;
    const int w = std::min(n - maxP, 4096);
    if (w < minP) return 0;
    double e0 = 0.0;
    for (int i = 0; i < w; ++i) e0 += (double) x[i] * x[i];
    if (e0 < 1e-12) return 0;
    double bestR = 0.0;
    int best = 0;
    auto corr = [&](int p) {
        double num = 0.0, ep = 0.0;
        for (int i = 0; i < w; ++i) {
            num += (double) x[i] * x[i + p];
            ep += (double) x[i + p] * x[i + p];
        }
        return ep > 1e-12 ? num / std::sqrt(e0 * ep) : 0.0;
    };
    for (int p = minP; p <= maxP; ++p) {
        const double r = corr(p);
        if (r > bestR) { bestR = r; best = p; }
    }
    if (bestR < 0.5) return 0;
    for (int d = best / minP; d >= 2; --d) {
        const int p0 = (int) ((double) best / (double) d + 0.5);
        double subBest = 0.0;
        int subP = 0;
        for (int p = std::max(minP, p0 - 1); p <= std::min(maxP, p0 + 1); ++p) {
            const double r = corr(p);
            if (r > subBest) { subBest = r; subP = p; }
        }
        if (subBest > 0.9 * bestR) return subP;
    }
    return best;
}

inline bool waveTableFromSignal(const float* x, int n, double sampleRate,
                                float* out, int len) {
    const int minP = (int) (sampleRate / 1000.0);
    const int maxP = (int) (sampleRate / 30.0);
    const int p = waveTableEstimatePeriod(x, n, minP, maxP);
    if (p <= 0) return false;
    int i0 = 0;
    for (int i = 0; i + 1 < p; ++i)
        if (x[i] <= 0.0f && x[i + 1] > 0.0f) { i0 = i; break; }
    waveTableResample(x + i0, p, out, len);
    waveTableTame(out, len);
    return true;
}

inline void waveTableAlignFrames(float* frames, int count, int len) {
    if (count < 2) return;
    auto fundPhase = [&](const float* f) {
        double c = 0.0, s = 0.0;
        for (int k = 0; k < len; ++k) {
            const double a = 2.0 * 3.14159265358979323846 * k / len;
            c += f[k] * std::cos(a);
            s += f[k] * std::sin(a);
        }
        return std::atan2(s, c);
    };
    const double p0 = fundPhase(frames);
    std::vector<float> tmp((std::size_t) len);
    for (int fi = 1; fi < count; ++fi) {
        float* f = frames + (std::size_t) fi * len;
        const double shift = (p0 - fundPhase(f)) / (2.0 * 3.14159265358979323846) * len;
        for (int k = 0; k < len; ++k) {
            double src = k + shift;
            src -= std::floor(src / len) * len;
            const int i0 = (int) src;
            const int i1 = (i0 + 1) % len;
            const float t = (float) (src - i0);
            tmp[(std::size_t) k] = f[i0] * (1.0f - t) + f[i1] * t;
        }
        std::copy(tmp.begin(), tmp.end(), f);
    }
}

inline int decodeWaveFrames(const char* s, float* out, int len, int maxFrames) {
    if (s == nullptr || maxFrames < 1) return 0;
    const std::string str(s);
    int count = 0;
    std::size_t start = 0;
    while (count < maxFrames) {
        const std::size_t bar = str.find('|', start);
        const std::string seg = str.substr(start, bar == std::string::npos ? std::string::npos
                                                                            : bar - start);
        if (!decodeWaveTable(seg.c_str(), out + (std::size_t) count * len, len)) break;
        ++count;
        if (bar == std::string::npos) break;
        start = bar + 1;
    }
    return count;
}

inline std::string encodeWaveFrames(const float* frames, int count, int len) {
    std::string s;
    for (int f = 0; f < count; ++f) {
        if (f > 0) s += "|";
        s += encodeWaveTable(frames + (std::size_t) f * len, len);
    }
    return s;
}

inline int waveTableFramesFromSignal(const float* x, int n, double sampleRate,
                                     float* out, int len, int maxFrames) {
    if (n < 2 * len || maxFrames < 2)
        return waveTableFromSignal(x, n, sampleRate, out, len) ? 1 : 0;
    const int frames = maxFrames;
    const int win = n / frames;
    for (int f = 0; f < frames; ++f) {
        float* dst = out + (std::size_t) f * len;
        if (!waveTableFromSignal(x + f * win, win, sampleRate, dst, len))
            waveTableFromCycle(x + f * win, win, dst, len);
    }
    return frames;
}

}
