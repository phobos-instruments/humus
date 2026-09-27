// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <utility>

#include "core/timeline/TapTempo.h"
#include "gui/editor/ParamRanges.h"
#include "gui/editor/inputs/NumberInputs.h"
#include "gui/host/BrickHost.h"
#include "hum/PatternMatrix.h"

namespace hum::input {

class TapTempo {
public:
    TapTempo(BrickHost& host, std::string organism, std::string param, std::string offParam)
        : host_(host), organism_(std::move(organism)), param_(std::move(param)), off_(std::move(offParam)) {}

    bool tap(double nowSeconds) {
        const double ms = core_.tap(nowSeconds);
        if (ms <= 0.0) return false;
        if (!off_.empty() && offValue() >= 0.5) {
            host_.pushUndo();
            host_.setParam(organism_, off_, 0.0);
        }
        const auto range = paramRange(host_, organism_, param_);
        host_.editParam(organism_, param_, limit(range.first, range.second, ms));
        return true;
    }

private:
    double offValue() const {
        if (const auto* cm = host_.model().byName(organism_))
            for (const auto& pr : cm->properties)
                if (pr.name == off_) return pr.value;
        return 0.0;
    }

    BrickHost& host_;
    std::string organism_, param_, off_;
    TapTempoCore core_;
};

class StepNudge {
public:
    StepNudge(BrickHost& host, std::string organism, std::string param)
        : host_(host), organism_(std::move(organism)), param_(std::move(param)) {}

    int current() const { return (int) std::lround(host_.liveParamValue(organism_, param_)); }
    const std::string& param() const { return param_; }

    bool nudge(int by, int steps) { return set(wrappedNudge(current(), by, steps)); }

    bool set(int to) {
        if (to == current()) return false;
        host_.pushUndo();
        host_.setParam(organism_, param_, (double) to);
        return true;
    }

    std::string text() const {
        const int v = current();
        return v > 0 ? "+" + std::to_string(v) : std::to_string(v);
    }

private:
    BrickHost& host_;
    std::string organism_, param_;
};

}
