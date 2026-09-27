// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/inputs/NumberInputs.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace hum::input {

double NumberInput::shown() const {
    if (!shows_.empty()) {
        const live::Controls controls(host_, organism_);
        if (const auto* v = controls.find(shows_)) return v->value;
    }
    return setting();
}

bool NumberInput::set(double v) {
    v = limit(range_.lo, range_.hi, range_.integer ? std::round(v) : v);
    if (v == setting()) return false;
    host_.editParam(organism_, param_, v);
    return true;
}

bool NumberInput::poll() {
    const double v = shown();
    if (v == last_) return false;
    last_ = v;
    return true;
}

double NumberInput::fieldDragPerPixel(bool fine, double fineFactor) const {
    double perPx = (range_.hi - range_.lo) / 200.0;
    if (fine) perPx /= fineFactor;
    if (range_.integer) perPx = std::max(perPx, fine ? 0.5 / fineFactor : 0.25);
    return perPx;
}

std::string NumberBox::text(double v) const {
    return number_.range().integer || decimals_ == 0 ? std::to_string((long long) std::llround(v))
                                                    : decimalText(v, decimals_);
}

NoteInput::NoteInput(ModelHost& host, std::string organism, std::string param, int lowest, int highest)
    : host_(host), organism_(std::move(organism)), param_(std::move(param)) {
    const auto r = rangeFor(host_, organism_, param_, {(double) lowest, (double) highest, true});
    lo_ = (int) r.lo;
    hi_ = (int) r.hi;
    last_ = note();
}

bool NoteInput::step(int direction) {
    const int n = note();
    const int next = (int) limit(lo_, hi_, n + direction);
    if (next == n) return false;
    host_.editParam(organism_, param_, (double) next);
    return true;
}

bool NoteInput::poll() {
    const int v = note();
    if (v == last_) return false;
    last_ = v;
    return true;
}

const std::vector<std::string>& RhythmicUnit::units() {
    static const std::vector<std::string> all = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"};
    return all;
}

std::string RhythmicUnit::unit() const {
    auto u = host_.liveParamText(organism_, unit_);
    return u.empty() ? kDefaultUnit : u;
}

}
