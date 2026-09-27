// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/Faceplate.h"

#include <algorithm>

#include "gui/editor/ChoiceModel.h"
#include "gui/editor/ControlModel.h"
#include "gui/editor/KnobModel.h"
#include "gui/editor/MomentaryModel.h"
#include "gui/editor/MomentaryPress.h"
#include "gui/editor/StepperModel.h"
#include "gui/editor/StrandLamp.h"
#include "gui/editor/ToggleModel.h"
#include "gui/tracks/StripSources.h"

namespace hum {

void Faceplate::buildControl(const LayoutSpec::Control& s, Control& c, int family) {
    using CT = LayoutSpec::ControlType;
    switch (s.type) {
        case CT::Momentary: buildMomentary(s, c); return;
        case CT::Knob: case CT::VSlider: case CT::HSlider: buildKnob(s, c, family); return;
        case CT::RotarySwitch: buildRotarySwitch(s, c, family); return;
        case CT::Toggle: case CT::MiniToggle: case CT::LitButton: buildToggle(s, c, family); return;
        case CT::EnumButtons: buildEnumButtons(s, c, family); return;
        case CT::Combo: buildCombo(s, c); return;
        case CT::IntSpinner: case CT::DoubleSpinner: buildSpinner(s, c); return;
        default: c.rich = views_.rich(spec_, s, *this); return;
    }
}

void Faceplate::buildMomentary(const LayoutSpec::Control& s, Control& c) {
    auto setup = momentary::setupFor(s, name_);
    setup.art = art::forControl(s, spec_.dir);
    auto view = views_.momentary(setup);
    view->onMenu = [this, param = s.param](Point at) { openMenu(param, at); };
    view->onWrite = [this, setup](double value) { host_.setParam(setup.organism, setup.param, value); };
    view->coverTicks = [this] { return owner_.coverTicks(); };
    auto* w = view.get();
    c.momentary = std::move(view);
    c.refreshMomentary = [this, w, setup] {
        const auto st = momentary::stateOf(host_, setup);
        if (!setup.lamp.active()) {
            w->showHeld(st.held);
            return;
        }
        const unsigned now = owner_.nowMs();
        w->showLamp(momentary::lampShows(st, now / strandlamp::kBlinkMs % 2 == 0), st.tint, st.dim);
    };
}

void Faceplate::buildKnob(const LayoutSpec::Control& s, Control& c, int family) {
    auto setup = knob::setupFor(s, name_, className());
    setup.art = art::forControl(s, spec_.dir);
    auto view = views_.knob(setup, {family, setup.isInt ? 0 : s.decimalPlaces, resetValueFor(setup.param)});
    if (setup.meterChannel >= 0) {
        c.meterChannel = setup.meterChannel;
        meters_ = true;
    }
    view->showValue(knob::valueOf(host_, setup));
    c.showValue = setup.showsValueInLabel && c.label != nullptr;
    auto* vp = view.get();
    auto* lb = c.showValue ? c.label.get() : nullptr;
    if (lb) lb->showText(vp->shownText());
    vp->onIntent = [this, vp, setup, lb](knob::Intent intent, double value) {
        knob::apply(host_, setup, intent, value);
        if (intent == knob::Intent::DragTo && lb) lb->showText(vp->shownText());
    };
    vp->onMenu = [this, param = setup.param](Point at) { openMenu(param, at); };
    vp->tooltipText = [this, setup] {
        const auto strip = stripChannelInlets(setup.param).empty() ? std::string() : knob::tooltipOf(host_, setup);
        const auto control = host_.controlSummary(setup.organism, setup.param);
        return strip.empty() || control.empty() ? strip + control : strip + "\n" + control;
    };
    c.value = std::move(view);
}

void Faceplate::buildRotarySwitch(const LayoutSpec::Control& s, Control& c, int family) {
    auto setup = knob::setupFor(s, name_, className());
    setup.art = art::forControl(s, spec_.dir);
    auto view = views_.rotarySwitch(setup, choice::setupFor(s, name_), {family, 0, resetValueFor(s.param)});
    view->showValue(control::paramValue(host_, name_, s.param));
    view->onIntent = [this, setup](knob::Intent intent, double value) { knob::apply(host_, setup, intent, value); };
    view->onMenu = [this, param = s.param](Point at) { openMenu(param, at); };
    c.value = std::move(view);
}

void Faceplate::buildToggle(const LayoutSpec::Control& s, Control& c, int family) {
    auto setup = toggle::setupFor(s, name_);
    setup.art = art::forControl(s, spec_.dir);
    setup.family = family;
    auto view = views_.toggle(setup);
    view->showOn(toggle::onOf(host_, setup));
    view->onToggle = [this, setup](bool on) {
        toggle::apply(host_, setup, on);
        applyDims();
    };
    view->onMenu = [this, param = setup.param](Point at) { openMenu(param, at); };
    c.toggle = std::move(view);
}

void Faceplate::buildEnumButtons(const LayoutSpec::Control& s, Control& c, int family) {
    auto setup = choice::setupFor(s, name_);
    setup.art = art::forControl(s, spec_.dir);
    c.enumFirst = setup.first;
    auto view = views_.choiceButtons(setup, family);
    view->showIndex(choice::indexForValue(setup, control::paramValue(host_, name_, s.param)));
    auto* vp = view.get();
    vp->onChoose = [this, vp, setup](int index) {
        choice::choose(host_, setup, index);
        vp->showIndex(index);
        applyDims();
    };
    vp->onMenu = [this, param = s.param](Point at) { openMenu(param, at); };
    c.buttons = std::move(view);
}

void Faceplate::buildSpinner(const LayoutSpec::Control& s, Control& c) {
    const auto setup = stepper::setupFor(s, name_, className());
    auto view = views_.spinner(setup, {-1, setup.decimals, resetValueFor(s.param)});
    for (double sample : stepper::widthSamples(setup))
        c.textBoxWant = std::max(c.textBoxWant, 8 + view->textWidthFor(stepper::measureText(setup, sample)));
    view->showValue(stepper::valueOf(host_, setup));
    view->onIntent = [this, setup](knob::Intent intent, double value) {
        if (intent == knob::Intent::DragStart) host_.beginParamDrag(setup.organism, setup.param);
        else if (intent == knob::Intent::DragEnd) host_.endParamDrag();
        else if (intent == knob::Intent::DragTo) host_.editParam(setup.organism, setup.param, value);
    };
    view->onMenu = [this, param = s.param](Point at) { openMenu(param, at); };
    c.value = std::move(view);
}

}
