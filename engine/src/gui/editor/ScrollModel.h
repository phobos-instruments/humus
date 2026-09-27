// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

namespace hum::scroll {

inline constexpr int kMinThumb = 18;
inline constexpr double kWheelPixels = 300.0;

inline double wheelPixels(double deltaY, bool reversed) { return -(reversed ? -deltaY : deltaY) * kWheelPixels; }

struct Thumb {
    int top = 0, height = 0;
};

struct Model {
    int content = 0, view = 0;
    double offset = 0.0;

    int maxOffset() const { return std::max(0, content - view); }
    bool scrollable() const { return maxOffset() > 0; }
    int position() const { return (int) std::lround(offset); }

    bool moveBy(double pixels) { return moveTo(offset + pixels); }
    bool moveTo(double wanted) {
        const double before = offset;
        offset = std::clamp(wanted, 0.0, (double) maxOffset());
        return std::abs(offset - before) > 1e-9;
    }
    void resize(int newContent, int newView) {
        content = newContent;
        view = newView;
        moveTo(offset);
    }

    Thumb thumb(int track) const {
        if (!scrollable() || content <= 0) return {0, track};
        const int height = std::min(track, std::max(kMinThumb, track * view / content));
        const int travel = track - height;
        return {(int) std::lround(travel * offset / maxOffset()), height};
    }
    double offsetForThumbTop(int top, int track) const {
        const int travel = track - thumb(track).height;
        if (travel <= 0) return 0.0;
        return std::clamp((double) top / travel, 0.0, 1.0) * maxOffset();
    }
};

}
