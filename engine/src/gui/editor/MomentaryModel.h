// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "gui/editor/ControlArt.h"
#include "gui/editor/ControlModel.h"
#include "gui/editor/StrandLamp.h"
#include "gui/host/ModelHost.h"
#include "hum/LayoutSpec.h"
#include "hum/Organism.h"
#include "hum/caps/Files.h"

namespace hum::momentary {

struct Setup {
    std::string organism, param, label, icon;
    bool holdable = false;
    bool confirmHold = false;
    strandlamp::Rule lamp;
    ControlArt art;
};

struct State {
    bool held = false;
    bool lit = false;
    bool dim = false;
    bool pending = false;
    int tint = StrandStatus::kEmpty;
};

inline bool handles(LayoutSpec::ControlType type) {
    return type == LayoutSpec::ControlType::Momentary;
}

inline Setup setupFor(const LayoutSpec::Control& control, const std::string& organism) {
    Setup s;
    s.organism = organism;
    s.param = control.param;
    s.label = control.label;
    s.icon = control.extraOr("icon", "");
    s.holdable = control.extraOr("hold", "") == "true";
    s.confirmHold = control.extraOr("confirm-hold", "") == "true";
    s.lamp = strandlamp::parse(control.extraOr("strand", ""), control.extraOr("lit-in", ""),
                               control.extraOr("dim-in", ""), control.extraOr("pending", ""));
    return s;
}

inline bool heldOf(ModelHost& host, const Setup& setup) {
    return host.isLiveTracked(setup.organism, setup.param)
           && host.liveParamValue(setup.organism, setup.param) >= 0.5;
}

inline State stateOf(ModelHost& host, const Setup& setup) {
    State st;
    st.held = heldOf(host, setup);
    if (!setup.lamp.active()) return st;

    auto* organism = host.liveOrganism(setup.organism);
    const auto* status = dynamic_cast<const StrandStatus*>(organism);
    const auto* intent = dynamic_cast<const StrandIntent*>(organism);
    const auto lamp = strandlamp::evaluate(
        setup.lamp,
        status != nullptr ? status->strandState(setup.lamp.strand) : (int) StrandStatus::kEmpty,
        intent != nullptr ? intent->strandPendingPress(setup.lamp.strand)
                          : (int) StrandIntent::kPressNone,
        intent != nullptr && intent->strandCanUndo(setup.lamp.strand),
        intent != nullptr && intent->strandCanRedo(setup.lamp.strand));
    st.lit = lamp.lit;
    st.dim = lamp.dim;
    st.pending = lamp.pending;
    st.tint = lamp.tint;
    return st;
}

inline bool lampShows(const State& state, bool blinkOn) {
    return state.held || state.lit || (state.pending && blinkOn);
}

}
