// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "gui/editor/inputs/NumberInputs.h"
#include "gui/host/BrickHost.h"

namespace hum::input {

class RangeInput {
public:
    enum class Target { None, Low, High, Both };

    RangeInput(BrickHost& host, std::string organism, std::string param, double min, double max, bool logarithmic);

    double low() const { return lo_; }
    double high() const { return hi_; }
    double minimum() const { return min_; }
    double maximum() const { return max_; }
    Target target() const { return target_; }
    const std::string& param() const { return param_; }

    bool refresh();
    float yOf(double v, float top, float bottom) const;
    void press(float y, float top, float bottom, bool fine);
    bool release();
    void dragAt(float y, float top, float bottom);
    void dragFineBy(float dy, float top, float bottom, double fineFactor);
    void wheel(double delta, bool fine);
    void setBoth(double lo, double hi);

    void aimAt(Target t) { target_ = t; }
    void nudgeCentre(double dFrac);
    void nudgeSpread(double dFrac);
    void nudgeEnd(bool high, double dFrac);

private:
    double fracOf(double v) const;
    double valueOf(double f) const;
    void moveBoth(double fLo, double fHi, double dFrac);

    void push() { host_.setParamRange(organism_, param_, lo_, hi_); }

    BrickHost& host_;
    std::string organism_, param_;
    double min_, max_;
    bool log_ = false;
    double lo_ = 0.0, hi_ = 1.0;
    Target target_ = Target::None;
    float anchorY_ = 0.0f;
    double anchorLo_ = 0.0, anchorHi_ = 0.0;
};

}
