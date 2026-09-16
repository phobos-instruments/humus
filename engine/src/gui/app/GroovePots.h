// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/host/EngineHost.h"
#include "gui/common/Localisation.h"
#include "gui/editor/ParamSlider.h"
#include "gui/app/ToolbarLadder.h"
#include "hum/Swing.h"
#include "io/PatchDocument.h"

namespace hum {

class GroovePots : public juce::Component {
public:
    static constexpr int kPotW = 30;
    static constexpr int kGrooveCaptionW = 42;
    static constexpr int kGridCaptionW = 28;
    static constexpr int kGap = 4;
    static constexpr int kWidth = kGrooveCaptionW + kPotW + kGap + kGridCaptionW + kPotW;
    static constexpr int kCompactWidth = kPotW + kGap + kPotW;
    static_assert(kWidth == toolbar::kGrooveW && kCompactWidth == toolbar::kGrooveCompactW);

    explicit GroovePots(EngineHost& host) : host_(host) {
        caption(grooveCaption_, tr("main-transport.groove", "Groove"));
        caption(gridCaption_, tr("main-transport.grid", "Grid"));

        addAndMakeVisible(groove_);
        groove_.setRange(0.0, 1.0, 0.01);
        groove_.setUnit(Unit::Percent);
        groove_.paramLabel = tr("main-transport.groove", "Groove");
        groove_.setPopupDisplayEnabled(true, false, nullptr);
        groove_.tooltipProvider = [this] {
            return "Groove " + groove_.getTextFromValue(groove_.getValue())
                   + " - how late the offbeats land";
        };
        groove_.onValueChange = [this] { host_.performGroove(groove_.getValue()); };
        groove_.onPopup = [this](juce::Point<int> at) { if (onMenu) onMenu(kGrooveParam, at); };

        addAndMakeVisible(grid_);
        grid_.setRange(0.0, (double) (swing::kGridChoices - 1), 1.0);
        grid_.paramLabel = tr("main-transport.grid", "Grid");
        grid_.textFromValueFunction = [](double v) {
            return juce::String(swing::gridUnitAt(std::lround(v)));
        };
        grid_.setPopupDisplayEnabled(true, false, nullptr);
        grid_.tooltipProvider = [this] {
            return "Grid " + grid_.getTextFromValue(grid_.getValue())
                   + " - which notes the groove swings";
        };
        grid_.onValueChange = [this] {
            host_.performGrooveGrid((int) std::lround(grid_.getValue()));
        };
        grid_.onPopup = [this](juce::Point<int> at) { if (onMenu) onMenu(kGrooveGridParam, at); };

        refresh();
    }

    std::function<void(const std::string& param, juce::Point<int> screen)> onMenu;

    void refresh() {
        follow(groove_, host_.liveGroove(), kGrooveParam);
        follow(grid_, (double) host_.liveGrooveGrid(), kGrooveGridParam);
        const auto ink = host_.liveGroove() > 0.0 ? Palette::accent : Palette::textDim;
        if (grooveCaption_.findColour(juce::Label::textColourId) != ink)
            grooveCaption_.setColour(juce::Label::textColourId, ink);
    }

    void setCompact(bool compact) {
        if (compact_ == compact) return;
        compact_ = compact;
        grooveCaption_.setVisible(!compact);
        gridCaption_.setVisible(!compact);
        resized();
    }
    int preferredWidth() const { return compact_ ? kCompactWidth : kWidth; }

    void resized() override {
        auto r = getLocalBounds();
        if (!compact_) grooveCaption_.setBounds(r.removeFromLeft(kGrooveCaptionW));
        groove_.setBounds(r.removeFromLeft(kPotW));
        r.removeFromLeft(kGap);
        if (!compact_) gridCaption_.setBounds(r.removeFromLeft(kGridCaptionW));
        grid_.setBounds(r.removeFromLeft(kPotW));
    }

private:
    struct GridPot : ParamSlider {
        GridPot() : ParamSlider(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox) {}
        void mouseDoubleClick(const juce::MouseEvent&) override {
            setValue((double) swing::gridIndexOf(stepTicksFor("1/16")), juce::sendNotificationSync);
        }
    };

    void caption(juce::Label& l, const juce::String& text) {
        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setColour(juce::Label::textColourId, Palette::textDim);
        l.setFont(juce::FontOptions(11.0f));
        l.setJustificationType(juce::Justification::centredRight);
        l.setInterceptsMouseClicks(false, false);
    }

    void follow(ParamSlider& pot, double live, const char* param) {
        if (!pot.isMouseButtonDown() && std::abs(pot.getValue() - live) > 1e-4)
            pot.setValue(live, juce::dontSendNotification);
        const auto clock = host_.clockNodeNameIfAny();
        pot.setExternallyControlled(!clock.empty() && host_.isExternallyControlled(clock, param));
    }

    EngineHost& host_;
    bool compact_ = false;
    juce::Label grooveCaption_, gridCaption_;
    ParamSlider groove_{juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox};
    GridPot grid_;
};

}
