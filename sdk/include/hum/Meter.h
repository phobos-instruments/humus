// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace hum {

inline constexpr int kMeterMaxBeats = 64;
inline constexpr int kMeterMaxUnit = 64;

inline bool meterUnitValid(int unit) {
    return unit >= 1 && unit <= kMeterMaxUnit && (unit & (unit - 1)) == 0;
}

inline int meterUnitSnap(double unit) {
    int best = 4;
    double bestDist = 1.0e9;
    for (int u = 1; u <= kMeterMaxUnit; u *= 2) {
        const double d = std::abs((double) u - unit);
        if (d < bestDist) { bestDist = d; best = u; }
    }
    return best;
}

struct Meter {
    std::int32_t beats = 4;
    std::int32_t unit = 4;

    double quarterNotesPerBeat() const { return 4.0 / (double) (unit > 0 ? unit : 4); }
    double quarterNotesPerBar() const { return (double) beats * quarterNotesPerBeat(); }
    bool valid() const { return beats >= 1 && beats <= kMeterMaxBeats && meterUnitValid(unit); }
    bool operator==(const Meter& o) const { return beats == o.beats && unit == o.unit; }
    bool operator!=(const Meter& o) const { return !(*this == o); }

    static Meter clamped(double beats, double unit) {
        Meter m;
        const long b = std::lround(beats);
        m.beats = (std::int32_t) (b < 1 ? 1 : b > kMeterMaxBeats ? kMeterMaxBeats : b);
        m.unit = meterUnitSnap(unit);
        return m;
    }
};

inline std::string meterText(const Meter& m) {
    return std::to_string(m.beats) + "/" + std::to_string(m.unit);
}

inline bool parseMeterText(const std::string& text, Meter& out) {
    const auto slash = text.find('/');
    if (slash == std::string::npos) return false;
    const int beats = std::atoi(text.substr(0, slash).c_str());
    const int unit = std::atoi(text.substr(slash + 1).c_str());
    const Meter m{beats, unit};
    if (!m.valid()) return false;
    out = m;
    return true;
}

struct MeterChange {
    double beat = 0.0;
    Meter meter;
};

namespace meter {

inline int segmentIndexAt(const MeterChange* c, int n, double beat) {
    int i = 0;
    while (i + 1 < n && c[i + 1].beat <= beat) ++i;
    return i;
}

inline Meter at(const MeterChange* c, int n, double beat) {
    if (c == nullptr || n <= 0) return Meter{};
    return c[segmentIndexAt(c, n, beat)].meter;
}

inline double segmentStart(const MeterChange* c, int n, int i) {
    return i <= 0 || n <= 0 ? 0.0 : c[i].beat;
}

inline int barsInSegment(const MeterChange* c, int n, int i) {
    if (i + 1 >= n) return 0;
    const double span = c[i + 1].beat - segmentStart(c, n, i);
    return (int) std::ceil(span / c[i].meter.quarterNotesPerBar() - 1.0e-9);
}

inline double barStartBefore(const MeterChange* c, int n, double beat) {
    if (c == nullptr || n <= 0) return 4.0 * std::floor(beat / 4.0);
    const int i = segmentIndexAt(c, n, beat);
    const double s = segmentStart(c, n, i);
    const double qpb = c[i].meter.quarterNotesPerBar();
    return s + std::floor((beat - s) / qpb + 1.0e-9) * qpb;
}

inline double nextBarStart(const MeterChange* c, int n, double beat) {
    if (c == nullptr || n <= 0) return 4.0 * (std::floor(beat / 4.0 + 1.0e-9) + 1.0);
    const int i = segmentIndexAt(c, n, beat);
    const double s = segmentStart(c, n, i);
    const double qpb = c[i].meter.quarterNotesPerBar();
    const double next = s + (std::floor((beat - s) / qpb + 1.0e-9) + 1.0) * qpb;
    if (i + 1 < n && next > c[i + 1].beat) return c[i + 1].beat;
    return next;
}

inline int barAt(const MeterChange* c, int n, double beat) {
    if (c == nullptr || n <= 0) return 1 + (int) std::floor(beat / 4.0);
    const int i = segmentIndexAt(c, n, beat);
    int bars = 0;
    for (int k = 0; k < i; ++k) bars += barsInSegment(c, n, k);
    const double s = segmentStart(c, n, i);
    return 1 + bars + (int) std::floor((beat - s) / c[i].meter.quarterNotesPerBar() + 1.0e-9);
}

inline double barStart(const MeterChange* c, int n, int barIndex) {
    if (barIndex <= 0) return 0.0;
    if (c == nullptr || n <= 0) return 4.0 * barIndex;
    int remaining = barIndex;
    for (int i = 0; i < n; ++i) {
        const int inSeg = barsInSegment(c, n, i);
        if (i + 1 < n && remaining >= inSeg) { remaining -= inSeg; continue; }
        return segmentStart(c, n, i) + remaining * c[i].meter.quarterNotesPerBar();
    }
    return 0.0;
}

inline double beatInBar(const MeterChange* c, int n, double beat) {
    const Meter m = at(c, n, beat);
    const double s = barStartBefore(c, n, beat);
    return 1.0 + (beat - s) / m.quarterNotesPerBeat();
}

}

}
