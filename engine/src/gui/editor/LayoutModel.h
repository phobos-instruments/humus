// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include "gui/editor/Geometry.h"
#include "hum/LayoutSpec.h"

namespace hum::layout {

inline constexpr double kMaxGrow = 2.5;
inline constexpr int kLabelGap = 2;
inline constexpr int kSideLabelGap = 4;
inline constexpr int kDefaultTextBox = 48;

struct Fit {
    double kx = 1.0, ky = 1.0;
    int xOff = 0;
};

inline bool extraTrue(const std::string& value) { return value == "1" || value == "true"; }

inline double scaleFor(const LayoutSpec& spec, int width) {
    if (spec.width <= 0) return 1.0;
    const double want = (double) width / (double) spec.width;
    if (spec.resize == LayoutSpec::Resize::Grow) return std::min(want, kMaxGrow);
    return std::min(1.0, want);
}

inline Fit fitFor(const LayoutSpec& spec, int width) {
    Fit f;
    const bool stretch = spec.resize == LayoutSpec::Resize::Stretch;
    f.kx = stretch ? (spec.width > 0 ? (double) width / spec.width : 1.0) : scaleFor(spec, width);
    f.ky = stretch ? 1.0 : f.kx;
    f.xOff = stretch ? 0
                     : std::max(0, (width - (int) std::lround(spec.width * f.kx)) / 2);
    return f;
}

inline int scaledX(const Fit& fit, int value) { return (int) std::lround(value * fit.kx); }
inline int scaledY(const Fit& fit, int value) { return (int) std::lround(value * fit.ky); }

inline int labelHeightFor(const Fit& fit) {
    return std::clamp((int) std::lround(14 * fit.ky), 11, 18);
}

inline int textBoxHeightFor(const Fit& fit) {
    return fit.ky >= 0.95 ? 20 : fit.ky >= 0.75 ? 16 : 14;
}

inline int fillControlIndex(const LayoutSpec& spec) {
    for (size_t i = 0; i < spec.controls.size(); ++i)
        if (extraTrue(spec.controls[i].extraOr("fill-v"))) return (int) i;
    return -1;
}

inline int slackFor(const LayoutSpec& spec, const Fit& fit, int height, int collarHeight) {
    if (fillControlIndex(spec) < 0) return 0;
    return std::max(0, height - collarHeight - scaledY(fit, spec.height));
}

inline Rect boundsFor(const LayoutSpec& spec, int index, const Fit& fit, int collarHeight,
                      int slack, int fillIndex) {
    const auto& control = spec.controls[(size_t) index];
    const int fillBottom = fillIndex < 0 ? 0
                                         : spec.controls[(size_t) fillIndex].y
                                               + spec.controls[(size_t) fillIndex].h;
    const int dy = (fillIndex >= 0 && control.y >= fillBottom) ? slack : 0;
    const int dh = (index == fillIndex) ? slack : 0;
    return {fit.xOff + scaledX(fit, control.x), collarHeight + scaledY(fit, control.y) + dy,
            scaledX(fit, control.w), scaledY(fit, control.h) + dh};
}

inline bool labelSitsAbove(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    return type == CT::Knob || type == CT::VSlider || type == CT::RangeVSlider
           || type == CT::RangeKnob || type == CT::RotarySwitch || type == CT::LitButton;
}

inline int sideLabelWidth(const Fit& fit, int want, int boundsWidth) {
    return std::clamp(std::min(want, scaledX(fit, 72)), boundsWidth / 5, boundsWidth / 2);
}

inline bool textBoxWidened(int want) { return want > kDefaultTextBox; }

inline int textBoxWidth(int want, int boundsWidth) {
    return std::clamp(want, kDefaultTextBox,
                      std::max(kDefaultTextBox, (boundsWidth - 40) / 8 * 8));
}

inline int stepperArrowWidth(int boundsWidth) { return std::min(18, boundsWidth / 6); }

inline int radioCellWidth(int boundsWidth, int count) {
    return count > 0 ? boundsWidth / count : boundsWidth;
}

inline constexpr float kDimAlpha = 0.35f;
inline constexpr int kLatchUnset = -1;

struct DimState {
    bool dimmed = false;
    bool enabled = true;
};

inline DimState dimStateFor(bool conditionHolds) { return {conditionHolds, !conditionHolds}; }

inline float alphaFor(bool dimmed) { return dimmed ? kDimAlpha : 1.0f; }

inline bool visibleFor(bool hasShowRule, bool conditionHolds) {
    return !hasShowRule || conditionHolds;
}

inline int nextLatch(bool conditionHolds) { return conditionHolds ? 1 : 0; }

inline bool clearFires(int wasLatched, bool conditionHolds, bool paramHasText) {
    return wasLatched != kLatchUnset && wasLatched != 1 && conditionHolds && paramHasText;
}

}
