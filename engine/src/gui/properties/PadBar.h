// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

namespace hum::padbar {

inline constexpr int kTapeTop = 8, kTapeH = 4, kBarH = 3, kGripSlack = 5, kGripBand = 12;

struct Span {
    double lo = 0.0, hi = 0.0;
    double length() const { return std::max(1.0e-6, hi - lo); }
};

enum class Grip { None, In, Out };

inline Span clipSpan(double in, double out, double len) {
    Span s;
    s.lo = std::clamp(in, 0.0, std::max(0.0, len));
    s.hi = out > in ? std::min(out, len) : len;
    s.hi = std::max(s.hi, s.lo);
    return s;
}

inline double xOfTape(double seconds, int width, double len) {
    if (len <= 0.0 || width <= 0) return 0.0;
    return std::clamp(seconds / len, 0.0, 1.0) * width;
}

inline double tapeTimeAtX(double x, int width, double len) {
    if (width <= 0) return 0.0;
    return std::clamp(x / width, 0.0, 1.0) * std::max(0.0, len);
}

inline double clipTimeAtX(double x, int width, const Span& clip) {
    if (width <= 0) return clip.lo;
    return clip.lo + std::clamp(x / width, 0.0, 1.0) * clip.length();
}

inline Grip gripAt(double x, int y, int width, int thumbH, const Span& clip, double len) {
    if (len <= 0.0 || y < thumbH - kGripBand || y > thumbH) return Grip::None;
    const double dIn = std::abs(x - xOfTape(clip.lo, width, len));
    const double dOut = std::abs(x - xOfTape(clip.hi, width, len));
    if (std::min(dIn, dOut) > kGripSlack) return Grip::None;
    return dIn < dOut ? Grip::In : Grip::Out;
}

inline double dragIn(double seconds, double out, double len) {
    const double top = out > 0.0 ? std::min(out, len) : len;
    return std::clamp(seconds, 0.0, std::max(0.0, top - 0.05));
}

inline double dragOut(double seconds, double in, double len) {
    return std::clamp(seconds, std::min(in + 0.05, len), std::max(0.0, len));
}

}
