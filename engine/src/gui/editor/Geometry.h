// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace hum {

template <class T>
struct BasicPoint {
    T x = 0, y = 0;
    bool operator==(const BasicPoint& o) const { return x == o.x && y == o.y; }
    bool operator!=(const BasicPoint& o) const { return !(*this == o); }
};

template <class T>
struct BasicRect {
    T x = 0, y = 0, w = 0, h = 0;

    static BasicRect between(T x0, T y0, T x1, T y1) {
        return {x0 < x1 ? x0 : x1, y0 < y1 ? y0 : y1, x0 < x1 ? x1 - x0 : x0 - x1, y0 < y1 ? y1 - y0 : y0 - y1};
    }

    T right() const { return x + w; }
    T bottom() const { return y + h; }
    bool empty() const { return w <= 0 || h <= 0; }
    bool contains(T px, T py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    bool intersects(const BasicRect& o) const {
        return x + w > o.x && y + h > o.y && x < o.x + o.w && y < o.y + o.h && !empty() && !o.empty();
    }
    BasicRect expanded(T by) const { return {x - by, y - by, w + by + by, h + by + by}; }

    bool operator==(const BasicRect& o) const { return x == o.x && y == o.y && w == o.w && h == o.h; }
    bool operator!=(const BasicRect& o) const { return !(*this == o); }
};

using Point = BasicPoint<int>;
using Rect = BasicRect<int>;
using RectF = BasicRect<float>;

inline Rect takeTop(Rect& r, int amount) {
    const int h = amount < r.h ? amount : r.h;
    const Rect top{r.x, r.y, r.w, h};
    r.y += h;
    r.h -= h;
    return top;
}

inline Rect takeLeft(Rect& r, int amount) {
    const int w = amount < r.w ? amount : r.w;
    const Rect left{r.x, r.y, w, r.h};
    r.x += w;
    r.w -= w;
    return left;
}

}
