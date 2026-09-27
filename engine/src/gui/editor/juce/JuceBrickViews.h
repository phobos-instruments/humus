// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/views/BrickViews.h"
#include "gui/host/EditorHost.h"

namespace hum {

class JuceBrickViews : public BrickViews {
public:
    JuceBrickViews(juce::Component& parent, EditorHost& host) : parent_(parent), host_(host) {}

    std::unique_ptr<LabelView> label(LabelKind kind, const std::string& text) override;
    std::unique_ptr<ValueView> knob(const knob::Setup& setup, const ValueLook& look) override;
    std::unique_ptr<ValueView> rotarySwitch(const knob::Setup& setup, const choice::Setup& options,
                                            const ValueLook& look) override;
    std::unique_ptr<ValueView> spinner(const stepper::Setup& setup, const ValueLook& look) override;
    std::unique_ptr<ToggleView> toggle(const toggle::Setup& setup) override;
    std::unique_ptr<ChoiceButtonsView> choiceButtons(const choice::Setup& setup, int family) override;
    std::unique_ptr<ComboView> combo(const choice::Setup& setup) override;
    std::unique_ptr<MomentaryView> momentary(const momentary::Setup& setup) override;
    std::unique_ptr<RichView> rich(const LayoutSpec& spec, const LayoutSpec::Control& control,
                                   RichOwner& owner) override;

private:
    template <class View>
    std::unique_ptr<View> shown(std::unique_ptr<View> view) {
        if (auto* widget = dynamic_cast<juce::Component*>(view.get())) parent_.addAndMakeVisible(*widget);
        return view;
    }

    juce::Component& parent_;
    EditorHost& host_;
};

}
