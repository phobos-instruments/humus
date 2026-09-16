// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum::numfmt {

struct Shape {
    bool rounds;
    bool bounded;
    double lo, hi;
};

inline const Shape* shapes(int& count) {
    static const Shape kShapes[] = {
        {false, false, -1.0e9, 1.0e9},
        {true, false, -1.0e9, 1.0e9},
        {true, true, 0.0, 255.0},
        {true, true, -(double) kSevenBitMax - 1.0, (double) kSevenBitMax},
        {true, true, 0.0, 65535.0},
        {true, true, -32768.0, 32767.0},
    };
    count = (int) (sizeof(kShapes) / sizeof(kShapes[0]));
    return kShapes;
}

inline int formatCount() {
    int n = 0;
    shapes(n);
    return n;
}

inline const Shape& shapeOf(int format) {
    int n = 0;
    const Shape* k = shapes(n);
    return k[std::clamp(format, 0, n - 1)];
}

inline float heldIn(double raw, int format) {
    const Shape& s = shapeOf(format);
    return (float) std::clamp(s.rounds ? std::rint(raw) : raw, s.lo, s.hi);
}

inline float spreadAcross(double unit, int format) {
    const Shape& s = shapeOf(format);
    if (!s.bounded) return (float) (s.rounds ? std::rint(unit) : unit);
    return (float) std::clamp(std::rint(s.lo + unit * (s.hi - s.lo)), s.lo, s.hi);
}

}
