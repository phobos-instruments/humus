// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/inputs/RangeInput.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace hum::input {

RangeInput::RangeInput(BrickHost& host, std::string organism, std::string param, double min, double max, bool logarithmic)
    : host_(host), organism_(std::move(organism)), param_(std::move(param)), min_(min), max_(max), log_(logarithmic && min > 0.0 && max > min), lo_(host_.liveParamValue(organism_, param_)), hi_(host_.liveParamMax(organism_, param_)) {}

bool RangeInput::refresh() {
    const double lo = host_.liveParamValue(organism_, param_);
    const double hi = host_.liveParamMax(organism_, param_);
    if (lo_ == lo && hi_ == hi) return false;
    lo_ = lo;
    hi_ = hi;
    return true;
}

float RangeInput::yOf(double v, float top, float bottom) const {
    const double f = limit(0.0, 1.0, fracOf(v));
    return bottom - (float) f * (bottom - top);
}

void RangeInput::press(float y, float top, float bottom, bool fine) {
    const float yLow = yOf(lo_, top, bottom);
    const float yHigh = yOf(hi_, top, bottom);
    const float gap = yLow - yHigh;
    const float thumbR = 11.0f;
    anchorY_ = y;
    anchorLo_ = lo_;
    anchorHi_ = hi_;
    if (gap >= 24.0f) {
        target_ = y < yHigh + thumbR ? Target::High : y > yLow - thumbR ? Target::Low : Target::Both;
    } else {
        const float midY = 0.5f * (yHigh + yLow);
        const float h = std::max(15.0f, gap * 0.5f + 9.0f);
        target_ = y < midY - h / 3.0f ? Target::High : y > midY + h / 3.0f ? Target::Low : Target::Both;
    }
    host_.pushUndo();
    if (target_ != Target::Both && !fine) dragAt(y, top, bottom);
}

bool RangeInput::release() {
    if (target_ == Target::None) return false;
    target_ = Target::None;
    return true;
}

void RangeInput::dragAt(float y, float top, float bottom) {
    if (target_ == Target::Both) {
        const double denom = (double) (bottom - top);
        if (denom <= 0.0) return;
        moveBoth(fracOf(anchorLo_), fracOf(anchorHi_), (double) (anchorY_ - y) / denom);
        push();
        return;
    }
    const double v = valueOf((bottom - y) / (bottom - top));
    if (target_ == Target::Low) lo_ = std::min(v, hi_);
    else if (target_ == Target::High) hi_ = std::max(v, lo_);
    push();
}

void RangeInput::dragFineBy(float dy, float top, float bottom, double fineFactor) {
    const double denom = (double) (bottom - top);
    if (denom <= 0.0) return;
    const double dFrac = (double) dy / denom / fineFactor;
    const double fLo = fracOf(lo_), fHi = fracOf(hi_);
    if (target_ == Target::Both) {
        moveBoth(fLo, fHi, dFrac);
    } else if (target_ == Target::Low) {
        lo_ = valueOf(limit(0.0, fHi, fLo + dFrac));
    } else if (target_ == Target::High) {
        hi_ = valueOf(limit(fLo, 1.0, fHi + dFrac));
    }
    push();
}

void RangeInput::wheel(double delta, bool fine) {
    const double step = (max_ - min_) * (fine ? 0.001 : 0.01);
    lo_ = limit(min_, max_, lo_ + delta * step);
    hi_ = limit(min_, max_, hi_ + delta * step);
    push();
}

void RangeInput::setBoth(double lo, double hi) {
    host_.pushUndo();
    lo_ = lo;
    hi_ = hi;
    push();
}

double RangeInput::fracOf(double v) const {
    if (log_) return std::log(limit(min_, max_, v) / min_) / std::log(max_ / min_);
    return (v - min_) / (max_ - min_);
}

double RangeInput::valueOf(double f) const {
    f = limit(0.0, 1.0, f);
    if (log_) return min_ * std::pow(max_ / min_, f);
    return min_ + f * (max_ - min_);
}

void RangeInput::nudgeCentre(double dFrac) {
    moveBoth(fracOf(lo_), fracOf(hi_), dFrac);
    push();
}

void RangeInput::nudgeSpread(double dFrac) {
    const double fLo = fracOf(lo_), fHi = fracOf(hi_);
    const double centre = 0.5 * (fLo + fHi);
    const double half = limit(0.0, 0.5, 0.5 * (fHi - fLo) + dFrac);
    lo_ = valueOf(limit(0.0, 1.0, centre - half));
    hi_ = valueOf(limit(0.0, 1.0, centre + half));
    push();
}

void RangeInput::nudgeEnd(bool high, double dFrac) {
    const double fLo = fracOf(lo_), fHi = fracOf(hi_);
    if (high) hi_ = valueOf(limit(fLo, 1.0, fHi + dFrac));
    else lo_ = valueOf(limit(0.0, fHi, fLo + dFrac));
    push();
}

void RangeInput::moveBoth(double fLo, double fHi, double dFrac) {
    const double width = fHi - fLo;
    double nLo = fLo + dFrac, nHi = fHi + dFrac;
    if (nLo < 0.0) {
        nLo = 0.0;
        nHi = width;
    }
    if (nHi > 1.0) {
        nHi = 1.0;
        nLo = 1.0 - width;
    }
    lo_ = valueOf(nLo);
    hi_ = valueOf(nHi);
}

}
