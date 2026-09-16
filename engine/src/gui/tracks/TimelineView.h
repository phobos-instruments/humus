// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <cmath>

#include "hum/Pattern.h"

namespace hum {

struct TimeSelection {
    bool active = false;
    bool dragging = false;
    double from = 0.0, to = 0.0, anchor = 0.0;
};

struct TimelineView {
    double ppb = 26.0;
    double scrollBeats = 0.0;
    double playBeat = 0.0;
    double snapChoice = 0.0;
    bool follow = false;
    int vScroll = 0;
    TimeSelection sel;

    float beatToX(double beat, int originX) const {
        return (float) (originX + (beat - scrollBeats) * ppb);
    }
    double xToBeat(float x, int originX) const { return scrollBeats + (x - originX) / ppb; }
    float tickToX(int tick, int originX) const {
        return beatToX(tick / (double) Pattern::kTicksPerBeat, originX);
    }
    int xToTick(float x, int originX) const {
        return (int) std::llround(xToBeat(x, originX) * Pattern::kTicksPerBeat);
    }
};

}
