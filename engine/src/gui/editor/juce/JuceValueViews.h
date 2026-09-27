// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <optional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/Ember.h"
#include "gui/editor/ChoiceModel.h"
#include "gui/editor/KnobModel.h"
#include "gui/editor/ParamSlider.h"
#include "gui/editor/StepperModel.h"
#include "gui/editor/juce/JuceControlArt.h"
#include "gui/editor/juce/JuceView.h"
#include "gui/editor/views/ValueView.h"
#include "gui/style/Colours.h"

namespace hum {

class JuceValueView : public JuceView<ParamSlider, ValueView> {
public:
    JuceValueView(juce::Slider::SliderStyle style, juce::Slider::TextEntryBoxPosition box,
                  const std::string& organism, const std::string& param, const ValueLook& look)
        : JuceView<ParamSlider, ValueView>(style, box) {
        if (look.family >= 0) getProperties().set("family", look.family);
        if (look.resetTo) setDoubleClickReturnValue(true, *look.resetTo);
        onRestyle = [this] {
            setColour(juce::Slider::textBoxTextColourId, Palette::text);
            setColour(juce::Slider::textBoxBackgroundColourId, Palette::background);
            setColour(juce::Slider::textBoxOutlineColourId, Palette::border);
        };
        setParamId(organism, param);
        onValueChange = [this] { if (onIntent) onIntent(knob::Intent::DragTo, getValue()); };
        onDragStart = [this] { if (onIntent) onIntent(knob::Intent::DragStart, 0.0); };
        onDragEnd = [this] { if (onIntent) onIntent(knob::Intent::DragEnd, 0.0); };
        onPopup = [this](juce::Point<int> at) { if (onMenu) onMenu(pointOf(at)); };
    }

    void setArt(const ControlArt& art) {
        if (art.empty()) return;
        if (KnobArt loaded(art); loaded.valid()) art_ = std::move(loaded);
        repaint();
    }

    void paint(juce::Graphics& g) override {
        if (!art_) {
            ParamSlider::paint(g);
            return;
        }
        const auto area = getLookAndFeel().getSliderLayout(*this).sliderBounds.toFloat();
        const auto kind = getSliderStyle();
        if (kind == juce::Slider::LinearHorizontal || kind == juce::Slider::LinearVertical)
            art_->paintLinear(g, area, (float) getPositionOfValue(getValue()), kind == juce::Slider::LinearVertical);
        else
            art_->paintRotary(g, area, (float) valueToProportionOfLength(getValue()));
    }

    void showValue(double value) override { setValue(value, juce::dontSendNotification); }
    void showLiveValue(double value) override {
        setValue(value, juce::dontSendNotification);
        ember::stamp(*this);
    }
    double shownValue() const override { return getValue(); }
    std::string shownText() override { return getTextFromValue(getValue()).toStdString(); }
    void showMeter(float level) override { setMeterLevel(level); }
    void showMarks(bool externallyControlled, bool rollLocked) override {
        setExternallyControlled(externallyControlled);
        setRollLocked(rollLocked);
    }
    int textWidthFor(const std::string& text) const override {
        const juce::Font boxFont(juce::FontOptions(14.0f));
        return (int) std::ceil(juce::TextLayout::getStringWidth(boxFont, viewText(text)));
    }
    void setTextBox(int width, int height) override {
        setTextBoxStyle(juce::Slider::TextBoxLeft, false, width, height);
    }

    juce::String getTooltip() override {
        const auto own = ParamSlider::getTooltip();
        const auto extra = tooltipText ? viewText(tooltipText()) : juce::String();
        return own.isEmpty() || extra.isEmpty() ? own + extra : own + "\n" + extra;
    }

private:
    std::optional<KnobArt> art_;
};

inline std::unique_ptr<JuceValueView> makeJuceKnob(const knob::Setup& setup, const ValueLook& look,
                                                  juce::Component& popupParent) {
    const auto style = setup.style == knob::Style::Knob ? juce::Slider::RotaryVerticalDrag
                       : setup.style == knob::Style::VerticalFader ? juce::Slider::LinearVertical
                                                                   : juce::Slider::LinearHorizontal;
    auto view = std::make_unique<JuceValueView>(
        style, setup.showsTextBox ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox,
        setup.organism, setup.param, look);
    view->paramLabel = viewText(setup.label);
    view->setRange(setup.lo, setup.hi, 0.0);
    if (setup.logarithmic) view->configureScaling();
    view->setNumDecimalPlacesToDisplay(look.decimals);
    view->setUnit(setup.unit);
    if (setup.style != knob::Style::Knob) view->setPopupDisplayEnabled(true, true, &popupParent);
    view->setArt(setup.art);
    if (setup.centreFill) view->getProperties().set(kCentreFill, true);
    return view;
}

inline std::unique_ptr<JuceValueView> makeJuceRotarySwitch(const knob::Setup& setup,
                                                          const choice::Setup& options,
                                                          const ValueLook& look) {
    auto view = std::make_unique<JuceValueView>(juce::Slider::RotaryVerticalDrag,
                                                juce::Slider::TextBoxBelow, setup.organism,
                                                setup.param, look);
    view->paramLabel = viewText(setup.label);
    view->setRange(setup.lo, setup.hi, 1.0);
    view->textFromValueFunction = [options](double value) {
        return viewText(choice::optionTextFor(options, value));
    };
    view->updateText();
    view->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 56, 16);
    view->onRestyle();
    view->setArt(setup.art);
    return view;
}

inline std::unique_ptr<JuceValueView> makeJuceSpinner(const stepper::Setup& setup,
                                                     const ValueLook& look) {
    auto view = std::make_unique<JuceValueView>(juce::Slider::IncDecButtons,
                                                juce::Slider::TextBoxLeft, setup.organism,
                                                setup.param, look);
    view->paramLabel = viewText(setup.label);
    view->onRestyle();
    view->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 20);
    view->setRange(setup.lo, setup.hi, stepper::rangeStepOf(setup));
    if (stepper::carriesItsOwnStepping(setup)) view->setStepping(setup.step, setup.ladder);
    view->setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_Vertical);
    view->setNumDecimalPlacesToDisplay(setup.decimals);
    view->setUnit(setup.unit);
    return view;
}

}
