// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

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
