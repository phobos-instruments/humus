// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "core/params/ParamSchema.h"
#include "core/params/UnitText.h"
#include "core/params/ValueText.h"
#include "gui/editor/ControlModel.h"
#include "gui/editor/LayoutCondition.h"
#include "gui/host/ModelHost.h"
#include "hum/LayoutSpec.h"

namespace hum::stepper {

enum class Kind { Whole, Fractional };

struct Setup {
    std::string organism, param, label;
    double lo = 0.0, hi = 1.0;
    double step = 0.0;
    std::vector<double> ladder;
    bool isInt = false;
    int decimals = 2;
    Unit unit = Unit::None;
    Kind kind = Kind::Whole;
};

struct State {
    double value = 0.0;
    std::string text;
    bool enabled = true;
    bool dimmed = false;
    bool visible = true;
};

inline bool handles(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    return type == CT::IntSpinner || type == CT::DoubleSpinner;
}

inline std::vector<double> ladderOf(const LayoutSpec::Control& control) {
    std::vector<double> rungs;
    const auto steps = control.extraOr("steps");
    for (size_t start = 0; start <= steps.size();) {
        auto end = steps.find_first_of(" ,", start);
        if (end == std::string::npos) end = steps.size();
        const auto one = steps.substr(start, end - start);
        if (one.find_first_not_of(" \t\n\r") != std::string::npos) rungs.push_back(leadingDouble(one));
        start = end + 1;
    }
    std::sort(rungs.begin(), rungs.end());
    return rungs;
}

inline Setup setupFor(const LayoutSpec::Control& control, const std::string& organism,
                      const std::string& className) {
    Setup s;
    s.organism = organism;
    s.param = control.param;
    s.label = control.label.empty() ? control.param : control.label;
    s.kind = control.type == LayoutSpec::ControlType::IntSpinner ? Kind::Whole : Kind::Fractional;
    for (const auto& d : schemaFor(className))
        if (d.name == control.param) {
            s.lo = d.min;
            s.hi = d.max;
            s.isInt = d.isInt;
            s.unit = unitResolve(d.name, d.unit, d.min, d.max);
            break;
        }
    s.decimals = s.isInt || s.kind == Kind::Whole ? 0 : control.decimalPlaces;
    s.ladder = ladderOf(control);
    if (s.kind == Kind::Fractional) s.step = (s.hi - s.lo) / 100.0;
    return s;
}

inline double rangeStepOf(const Setup& setup) {
    return setup.kind == Kind::Whole ? 1.0 : 0.0;
}

inline bool carriesItsOwnStepping(const Setup& setup) {
    return setup.step > 0.0 || !setup.ladder.empty();
}

inline double steppedFrom(const Setup& setup, double value, int dir) {
    double moved = value;
    if (setup.ladder.empty()) {
        moved += dir * setup.step;
    } else {
        size_t near = 0;
        for (size_t k = 1; k < setup.ladder.size(); ++k)
            if (std::abs(setup.ladder[k] - moved) < std::abs(setup.ladder[near] - moved)) near = k;
        if (dir > 0 && setup.ladder[near] <= moved + 1e-9)
            near = std::min(near + 1, setup.ladder.size() - 1);
        else if (dir < 0 && setup.ladder[near] >= moved - 1e-9)
            near = near > 0 ? near - 1 : 0;
        moved = setup.ladder[near];
    }
    return std::clamp(moved, setup.lo, setup.hi);
}

inline std::string textOf(const Setup& setup, double value) {
    return setup.unit == Unit::None ? smartValueText(value)
                                    : unitText(setup.unit, value, setup.lo, setup.hi);
}

inline std::string measureText(const Setup& setup, double value) {
    return unitText(setup.unit, value, setup.lo, setup.hi);
}

inline std::vector<double> widthSamples(const Setup& setup) {
    return {setup.lo, setup.hi, (setup.lo + setup.hi) * 0.5};
}

inline double valueOf(ModelHost& host, const Setup& setup) {
    return control::paramValue(host, setup.organism, setup.param);
}

inline double resetValueOf(ModelHost& host, const Setup& setup) {
    return control::defaultValue(host, setup.organism, setup.param);
}

inline State stateOf(ModelHost& host, const Setup& setup, const LayoutCondition& dimWhen,
                     const LayoutCondition& showWhen) {
    State st;
    st.value = valueOf(host, setup);
    st.text = textOf(setup, st.value);
    st.dimmed = control::holds(host, setup.organism, dimWhen);
    st.enabled = !st.dimmed;
    st.visible = showWhen.empty() || control::holds(host, setup.organism, showWhen);
    return st;
}

inline void setTo(ModelHost& host, const Setup& setup, double value) {
    host.editParam(setup.organism, setup.param, std::clamp(value, setup.lo, setup.hi));
}

inline void stepBy(ModelHost& host, const Setup& setup, int dir) {
    setTo(host, setup, steppedFrom(setup, valueOf(host, setup), dir));
}

}
