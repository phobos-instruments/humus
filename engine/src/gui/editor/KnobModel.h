// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>

#include "gui/editor/ControlArt.h"
#include "core/params/ParamSchema.h"
#include "core/params/UnitText.h"
#include "core/params/ValueText.h"
#include "gui/editor/ControlModel.h"
#include "gui/editor/LayoutCondition.h"
#include "gui/editor/ParamRanges.h"
#include "gui/host/ModelHost.h"
#include "gui/tracks/StripSources.h"
#include "hum/LayoutSpec.h"
#include "hum/Registry.h"
#include "io/PatchDocument.h"

namespace hum::knob {

enum class Style { Knob, VerticalFader, HorizontalFader };

struct Setup {
    std::string organism, param, label;
    double lo = 0.0, hi = 1.0;
    bool isInt = false;
    bool logarithmic = false;
    Unit unit = Unit::None;
    Style style = Style::Knob;
    bool showsTextBox = false;
    bool showsValueInLabel = false;
    int meterChannel = -1;
    bool centreFill = false;
    ControlArt art;
};

struct State {
    double value = 0.0;
    std::string text;
    bool enabled = true;
    bool dimmed = false;
    bool visible = true;
    bool externallyControlled = false;
    bool rollLocked = false;
    float meterLevel = 0.0f;
    std::string tooltip;
};

enum class Intent { DragStart, DragTo, DragEnd, Reset };

inline bool handles(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    return type == CT::Knob || type == CT::VSlider || type == CT::HSlider;
}

inline Style styleOf(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    if (type == CT::VSlider) return Style::VerticalFader;
    if (type == CT::HSlider) return Style::HorizontalFader;
    return Style::Knob;
}

inline Setup setupFor(const LayoutSpec::Control& control, const std::string& organism,
                      const std::string& className) {
    Setup s;
    s.organism = organism;
    s.param = control.param;
    s.label = control.label.empty() ? control.param : control.label;
    s.style = styleOf(control.type);
    s.showsTextBox = s.style == Style::Knob;
    s.logarithmic = control.logarithmic;
    const auto shown = control.extraOr("show-value");
    s.showsValueInLabel = shown == "1" || shown == "true";
    const auto centre = control.extraOr("centre-fill");
    s.centreFill = centre == "1" || centre == "true";
    if (const auto meter = control.extraOr("meter"); !meter.empty())
        s.meterChannel = std::atoi(meter.c_str());
    for (const auto& d : schemaFor(className))
        if (d.name == control.param) {
            s.lo = d.min;
            s.hi = d.max;
            s.isInt = d.isInt;
            s.unit = unitResolve(d.name, d.unit, d.min, d.max);
            break;
        }
    return s;
}

inline double valueOf(ModelHost& host, const Setup& setup) {
    return control::paramValue(host, setup.organism, setup.param);
}

inline double liveValueOf(ModelHost& host, const Setup& setup) {
    return host.liveParamValue(setup.organism, setup.param);
}

inline std::string textOf(const Setup& setup, double value) {
    return setup.unit == Unit::None ? smartValueText(value)
                                    : unitText(setup.unit, value, setup.lo, setup.hi);
}

inline double resetValueOf(ModelHost& host, const Setup& setup) {
    if (const auto* cm = host.model().byName(setup.organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == setup.param) return d.def;
    return setup.lo;
}

inline bool holds(ModelHost& host, const std::string& organism, const LayoutCondition& condition) {
    return control::holds(host, organism, condition);
}

inline std::string tooltipOf(ModelHost& host, const Setup& setup) {
    if (stripChannelInlets(setup.param).empty()) return {};
    const auto names = stripSourceNames(setup.param, host.model().connections, setup.organism);
    if (names.empty()) return host.translated("strip-hover.nothing-connected", "Nothing connected");
    return joinedSources(names);
}

inline State stateOf(ModelHost& host, const Setup& setup, const LayoutCondition& dimWhen,
                     const LayoutCondition& showWhen, const float* meters = nullptr,
                     int meterChannels = 0) {
    State st;
    st.value = valueOf(host, setup);
    st.text = textOf(setup, st.value);
    st.dimmed = holds(host, setup.organism, dimWhen);
    st.enabled = !st.dimmed;
    st.visible = showWhen.empty() || holds(host, setup.organism, showWhen);
    st.externallyControlled = paramIsControlled(host, setup.organism, setup.param);
    st.rollLocked = host.rollLocked(setup.organism, setup.param);
    if (meters != nullptr && setup.meterChannel >= 0 && setup.meterChannel < meterChannels)
        st.meterLevel = meters[setup.meterChannel];
    st.tooltip = tooltipOf(host, setup);
    return st;
}

inline void apply(ModelHost& host, const Setup& setup, Intent intent, double value = 0.0) {
    switch (intent) {
        case Intent::DragStart:
            host.beginParamDrag(setup.organism, setup.param);
            return;
        case Intent::DragTo:
            host.editParam(setup.organism, setup.param, std::clamp(value, setup.lo, setup.hi));
            return;
        case Intent::DragEnd:
            host.endParamDrag();
            return;
        case Intent::Reset:
            host.editParam(setup.organism, setup.param, resetValueOf(host, setup));
            return;
    }
}

}
