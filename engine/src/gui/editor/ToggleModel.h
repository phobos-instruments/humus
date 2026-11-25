// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "gui/editor/ControlArt.h"
#include "gui/editor/ControlModel.h"
#include "gui/editor/LayoutCondition.h"
#include "gui/host/ModelHost.h"
#include "hum/LayoutSpec.h"

namespace hum::toggle {

enum class Kind { Tick, Button, Lit };

struct Setup {
    std::string organism, param, label, onLabel;
    Kind kind = Kind::Tick;
    ControlArt art;
    std::string icon, onColour, offColour;
    int family = -1;
};

struct State {
    bool on = false;
    std::string text;
    bool enabled = true;
    bool dimmed = false;
    bool visible = true;
};

inline bool handles(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    return type == CT::Toggle || type == CT::MiniToggle || type == CT::LitButton;
}

inline Setup setupFor(const LayoutSpec::Control& control, const std::string& organism) {
    Setup s;
    s.organism = organism;
    s.param = control.param;
    s.label = control.label;
    s.onLabel = control.extraOr("on-label", "");
    using CT = LayoutSpec::ControlType;
    s.kind = control.type == CT::MiniToggle ? Kind::Button : control.type == CT::LitButton ? Kind::Lit : Kind::Tick;
    s.icon = control.extraOr("icon", "Power");
    s.onColour = control.extraOr("colour", "");
    s.offColour = control.extraOr("off-colour", "");
    return s;
}

inline bool onOf(ModelHost& host, const Setup& setup) {
    return control::paramValue(host, setup.organism, setup.param) >= 0.5;
}

inline std::string textOf(const Setup& setup, bool on) {
    return on && !setup.onLabel.empty() ? setup.onLabel : setup.label;
}

inline State stateOf(ModelHost& host, const Setup& setup, const LayoutCondition& dimWhen,
                     const LayoutCondition& showWhen) {
    State st;
    st.on = onOf(host, setup);
    st.text = textOf(setup, st.on);
    st.dimmed = control::holds(host, setup.organism, dimWhen);
    st.enabled = !st.dimmed;
    st.visible = showWhen.empty() || control::holds(host, setup.organism, showWhen);
    return st;
}

inline void apply(ModelHost& host, const Setup& setup, bool on) {
    host.editParam(setup.organism, setup.param, on ? 1.0 : 0.0);
}

inline constexpr const char* kGroupKey = "group";
inline constexpr const char* kSoloGroup = "solo";

struct Edit {
    std::string param;
    double value = 0.0;
};

inline std::vector<Edit> groupEdits(const LayoutSpec& spec, const std::string& param, bool on, bool alt, bool command) {
    std::string group;
    for (const auto& c : spec.controls)
        if (c.param == param && handles(c.type)) group = c.extraOr(kGroupKey, "");
    const bool only = command && group == kSoloGroup;
    if (group.empty() || (!alt && !only)) return {{param, on ? 1.0 : 0.0}};
    std::vector<Edit> out;
    for (const auto& c : spec.controls) {
        if (!handles(c.type) || c.extraOr(kGroupKey, "") != group) continue;
        if (std::any_of(out.begin(), out.end(), [&](const Edit& e) { return e.param == c.param; })) continue;
        const bool lit = only ? c.param == param : on;
        out.push_back({c.param, lit ? 1.0 : 0.0});
    }
    return out;
}

}

namespace hum::labelctl {

struct Setup {
    std::string text;
    bool pill = false;
    bool section = false;
    bool ownedByControl = false;
};

inline bool handles(LayoutSpec::ControlType type) {
    return type == LayoutSpec::ControlType::Label;
}

inline Setup setupFor(const LayoutSpec::Control& control) {
    Setup s;
    s.text = control.label;
    s.pill = control.extraOr("style") == "pill";
    s.section = control.extraOr("style") == "section";
    s.ownedByControl = !handles(control.type);
    return s;
}

}
