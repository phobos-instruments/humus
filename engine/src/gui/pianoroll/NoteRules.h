// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "gui/editor/Geometry.h"
#include "core/packs/Roles.h"
#include "io/PatchDocument.h"

namespace hum::noteedit {

inline int clampTo(int v, int lo, int hi) { return v < lo ? lo : (hi < v ? hi : v); }

inline constexpr int kEdgePx = 4;
inline constexpr int kMinTicks = 3;

enum class Tool { Pointer, Draw, Scissors, Eraser, Line };
inline constexpr int kToolCount = 5;

inline float edgeBand(float width) { return std::clamp(width * 0.25f, 2.0f, (float) kEdgePx); }

enum class Grab { Miss, Body, LeftEdge, RightEdge };

inline Grab grabAt(const RectF& note, float px, float py) {
    const float top = note.y - 1.0f, tall = note.h + 2.0f;
    const bool inside = px >= note.x && py >= top && px < note.right() && py < top + tall;
    if (!inside) return Grab::Miss;
    const float band = edgeBand(note.w);
    if (note.w <= 3.0f * band) return Grab::Body;
    if (px <= note.x + band) return Grab::LeftEdge;
    if (px >= note.right() - band) return Grab::RightEdge;
    return Grab::Body;
}

inline int snapDelta(int delta, int grid) {
    if (grid <= 0) return delta;
    return (int) std::lround((double) delta / (double) grid) * grid;
}

inline int octaveSteps(const PatchDocumentModel& m) {
    int steps = 12;
    for (const auto& cm : m.organisms) {
        if (!classHasRole(cm.classRaw, role::kTuning)) continue;
        double preset = 0.0, divisions = 12.0;
        for (const auto& p : cm.properties) {
            if (p.name == "Preset") preset = p.value;
            if (p.name == "Divisions") divisions = p.value;
        }
        if (preset != 0.0) return 0;
        steps = std::max(2, (int) std::lround(divisions));
    }
    return steps;
}

inline int resizeRight(int noteTick, int pointerTick, int minTicks) {
    return std::max(minTicks, pointerTick - noteTick);
}

struct HeadResize { int tick, lengthTicks; };
inline HeadResize resizeLeft(int noteTick, int noteLen, int pointerTick, int minTicks) {
    const int end = noteTick + noteLen;
    const int tick = std::clamp(pointerTick, 0, end - minTicks);
    return {tick, end - tick};
}

class WheelAccum {
public:
    int add(float delta, float gain) {
        acc_ += delta * gain;
        const int whole = (int) acc_;
        acc_ -= (float) whole;
        return whole;
    }
    void reset() { acc_ = 0.0f; }

private:
    float acc_ = 0.0f;
};

}

namespace hum::notecolour {

inline constexpr int kMixed = -2;

inline int shared(const std::vector<int>& colours) {
    if (colours.empty()) return kMixed;
    for (int c : colours)
        if (c != colours.front()) return kMixed;
    return colours.front();
}

}
