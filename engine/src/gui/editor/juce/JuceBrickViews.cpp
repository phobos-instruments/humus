// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/juce/JuceBrickViews.h"

#include "gui/editor/juce/JuceButtonViews.h"
#include "gui/editor/juce/JuceChoiceViews.h"
#include "gui/editor/juce/JuceRichView.h"
#include "gui/editor/juce/JuceValueViews.h"

namespace hum {

std::unique_ptr<LabelView> JuceBrickViews::label(LabelKind kind, const std::string& text) {
    return shown<LabelView>(makeJuceLabel(kind, text));
}

std::unique_ptr<ValueView> JuceBrickViews::knob(const knob::Setup& setup, const ValueLook& look) {
    return shown<ValueView>(makeJuceKnob(setup, look, parent_));
}

std::unique_ptr<ValueView> JuceBrickViews::rotarySwitch(const knob::Setup& setup,
                                                        const choice::Setup& options,
                                                        const ValueLook& look) {
    return shown<ValueView>(makeJuceRotarySwitch(setup, options, look));
}

std::unique_ptr<ValueView> JuceBrickViews::spinner(const stepper::Setup& setup, const ValueLook& look) {
    return shown<ValueView>(makeJuceSpinner(setup, look));
}

std::unique_ptr<ToggleView> JuceBrickViews::toggle(const toggle::Setup& setup) {
    return shown(makeJuceToggle(setup));
}

std::unique_ptr<ChoiceButtonsView> JuceBrickViews::choiceButtons(const choice::Setup& setup, int family) {
    return std::make_unique<JuceChoiceButtons>(parent_, setup, family);
}

std::unique_ptr<ComboView> JuceBrickViews::combo(const choice::Setup& setup) {
    return std::make_unique<JuceComboView>(parent_, setup.hasSteppers, setup.art);
}

std::unique_ptr<MomentaryView> JuceBrickViews::momentary(const momentary::Setup& setup) {
    return shown<MomentaryView>(std::make_unique<JuceMomentaryView>(setup));
}

std::unique_ptr<RichView> JuceBrickViews::rich(const LayoutSpec& spec, const LayoutSpec::Control& control,
                                               RichOwner& owner) {
    if (auto view = makeJucePlayerBrick(parent_, host_, spec, control, owner)) return view;
    if (auto view = makeJuceReadoutBrick(parent_, host_, spec, control, owner)) return view;
    return makeJucePickerBrick(parent_, host_, spec, control, owner);
}

}
