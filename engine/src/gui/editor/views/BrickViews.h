// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include "gui/editor/ChoiceModel.h"
#include "gui/editor/KnobModel.h"
#include "gui/editor/MomentaryModel.h"
#include "gui/editor/StepperModel.h"
#include "gui/editor/ToggleModel.h"
#include "hum/LayoutSpec.h"
#include "gui/editor/views/BrickView.h"
#include "gui/editor/views/ChoiceView.h"
#include "gui/editor/views/RichView.h"
#include "gui/editor/views/ToggleView.h"
#include "gui/editor/views/ValueView.h"

namespace hum {

class BrickViews {
public:
    virtual ~BrickViews() = default;

    virtual std::unique_ptr<LabelView> label(LabelKind kind, const std::string& text) = 0;
    virtual std::unique_ptr<ValueView> knob(const knob::Setup& setup, const ValueLook& look) = 0;
    virtual std::unique_ptr<ValueView> rotarySwitch(const knob::Setup& setup,
                                                    const choice::Setup& options,
                                                    const ValueLook& look) = 0;
    virtual std::unique_ptr<ValueView> spinner(const stepper::Setup& setup, const ValueLook& look) = 0;
    virtual std::unique_ptr<ToggleView> toggle(const toggle::Setup& setup) = 0;
    virtual std::unique_ptr<ChoiceButtonsView> choiceButtons(const choice::Setup& setup, int family) = 0;
    virtual std::unique_ptr<ComboView> combo(const choice::Setup& setup) = 0;
    virtual std::unique_ptr<MomentaryView> momentary(const momentary::Setup& setup) = 0;
    virtual std::unique_ptr<RichView> rich(const LayoutSpec& spec, const LayoutSpec::Control& control,
                                           RichOwner& owner) = 0;
};

}
