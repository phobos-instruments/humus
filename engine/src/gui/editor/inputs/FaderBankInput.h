// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "gui/editor/Geometry.h"
#include "gui/editor/ParamRanges.h"
#include "gui/editor/inputs/NumberInputs.h"
#include "gui/host/BrickHost.h"

namespace hum::input {

class FaderBankInput {
public:
    static constexpr int kLabelH = 13;

    struct Fader {
        std::string param;
        double min = 0.0, max = 1.0;
    };

    FaderBankInput(BrickHost& host, std::string organism, std::vector<Fader> faders)
        : host_(host), organism_(std::move(organism)), faders_(std::move(faders)) {}

    int count() const { return (int) faders_.size(); }
    const Fader& fader(int i) const { return faders_[(size_t) i]; }

    int columnAt(int x, int width) const {
        const int n = count();
        if (n == 0 || width <= 0) return -1;
        return std::clamp(x * n / width, 0, n - 1);
    }

    RectF slot(int i, int width, int height) const {
        const float cw = (float) width / (float) faders_.size();
        return {i * cw + cw * 0.2f, (float) (kLabelH + 4), cw * 0.6f, (float) (height - kLabelH - 8)};
    }

    double value(int i) const { return host_.liveParamValue(organism_, fader(i).param); }

    float yOf(int i, double v, int width, int height) const {
        const auto s = slot(i, width, height);
        const auto& f = fader(i);
        const double frac = (f.max > f.min) ? (v - f.min) / (f.max - f.min) : 0.0;
        return (float) (s.bottom() - limit(0.0, 1.0, frac) * s.h);
    }

    double valueAt(int i, float y, int width, int height) const {
        const auto s = slot(i, width, height);
        const auto& f = fader(i);
        const double frac = limit(0.0, 1.0, (double) ((s.bottom() - y) / s.h));
        return f.min + frac * (f.max - f.min);
    }

    void press() { host_.pushUndo(); }

    bool drawAt(int x, float y, int width, int height) {
        const int c = columnAt(x, width);
        if (c < 0) return false;
        host_.setParam(organism_, fader(c).param, valueAt(c, y, width, height));
        return true;
    }

    void dragFine(int column, float dy, int width, int height, double fineFactor) {
        const auto& f = fader(column);
        const double perPx = (f.max - f.min) / std::max(1.0f, slot(column, width, height).h);
        host_.setParam(organism_, f.param, limit(f.min, f.max, value(column) + dy * perPx / fineFactor));
    }

    bool reset(int column) {
        double def = 0.0, defMax = 0.0;
        if (!paramDefault(host_, organism_, fader(column).param, def, defMax)) return false;
        host_.editParam(organism_, fader(column).param, def);
        return true;
    }

    bool wheel(int column, double delta, bool fine) {
        const auto& f = fader(column);
        const double range = f.max - f.min;
        if (range <= 0.0) return false;
        const double step = range * (fine ? 0.001 : 0.01) * (delta > 0.0 ? 1.0 : -1.0);
        host_.editParam(organism_, f.param, limit(f.min, f.max, value(column) + step));
        return true;
    }

private:
    BrickHost& host_;
    std::string organism_;
    std::vector<Fader> faders_;
};

}
