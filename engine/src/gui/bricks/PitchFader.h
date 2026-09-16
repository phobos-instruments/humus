// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostDeck.h"
#include "gui/editor/FineDrag.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class HoldButton : public juce::TextButton {
public:
    using juce::TextButton::TextButton;
    std::function<void(bool)> onHold;
    void mouseDown(const juce::MouseEvent& e) override { juce::TextButton::mouseDown(e); if (onHold) onHold(true); }
    void mouseUp(const juce::MouseEvent& e) override { juce::TextButton::mouseUp(e); if (onHold) onHold(false); }
};

class PitchFader : public juce::Component, private juce::Timer {
public:
    static constexpr double kBend = 4.0;

    PitchFader(BrickHost& host, std::string organism, const Bindings& bound)
        : host_(host), name_(std::move(organism)),
          pitchParam_(bound(bind::kPitch)), rangeParam_(bound(bind::kPitchRange)),
          fader_(juce::Slider::LinearVertical, juce::Slider::NoTextBox),
          bendUp_("+"), bendDown_("-"), range_("8%") {
        const double r = juce::jmax(1.0, host_.liveParamValue(name_, rangeParam_));
        fader_.setRange(-r, r, 0.0);
        applyFineCrawl(fader_);
        fader_.setValue(host_.liveParamValue(name_, pitchParam_), juce::dontSendNotification);
        fader_.setDoubleClickReturnValue(true, 0.0);
        fader_.onValueChange = [this] { host_.editParam(name_, pitchParam_, fader_.getValue()); repaint(); };
        fader_.onDragStart = [this] { host_.beginParamDrag(name_, pitchParam_); };
        fader_.onDragEnd = [this] { host_.endParamDrag(); };
        addAndMakeVisible(fader_);

        auto styleBtn = [this](juce::Button& b) {
            b.setColour(juce::TextButton::buttonColourId, Palette::panelLight);
            b.setColour(juce::TextButton::textColourOffId, Palette::text);
            addAndMakeVisible(b);
        };
        styleBtn(bendUp_); styleBtn(bendDown_); styleBtn(range_);
        bendUp_.setTooltip(tr("pitch-fader.nudge-tempo-up-hold", "Nudge tempo up (hold)"));
        bendDown_.setTooltip(tr("pitch-fader.nudge-tempo-down-hold", "Nudge tempo down (hold)"));
        range_.setTooltip(tr("pitch-fader.pitch-range", "Pitch range"));
        bendUp_.onHold   = [this](bool d) { host_.decks().setBend(name_, d ?  kBend : 0.0); };
        bendDown_.onHold = [this](bool d) { host_.decks().setBend(name_, d ? -kBend : 0.0); };
        range_.onClick = [this] {
            const double cur = host_.liveParamValue(name_, rangeParam_);
            const double next = cur < 12.0 ? 16.0 : cur < 30.0 ? 50.0 : 8.0;
            host_.setParam(name_, rangeParam_, next);
        };
        startTimerHz(20);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(2);
        const int bw = juce::jmin(r.getWidth(), 60);
        auto mid = [&](juce::Rectangle<int> row) { return row.withSizeKeepingCentre(bw, row.getHeight()); };
        bendUp_.setBounds(mid(r.removeFromTop(22)));
        range_.setBounds(mid(r.removeFromBottom(20)));
        bendDown_.setBounds(mid(r.removeFromBottom(22)));
        r.removeFromTop(14);
        fader_.setBounds(r.withSizeKeepingCentre(juce::jmin(r.getWidth(), 44), r.getHeight()));
    }

    void paint(juce::Graphics& g) override {
        const double pct = host_.liveParamValue(name_, pitchParam_);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText((pct >= 0 ? "+" : "") + juce::String(pct, 1) + "%",
                   getLocalBounds().withTop(24).withHeight(14), juce::Justification::centred);
    }

private:
    void timerCallback() override {
        const double r = juce::jmax(1.0, host_.liveParamValue(name_, rangeParam_));
        if (std::abs(fader_.getMaximum() - r) > 1e-6) fader_.setRange(-r, r, 0.0);
        const double v = host_.liveParamValue(name_, pitchParam_);
        if (std::abs(fader_.getValue() - v) > 1e-6) fader_.setValue(v, juce::dontSendNotification);
        range_.setButtonText(juce::String((int) r) + "%");
        repaint();
    }

    BrickHost& host_;
    std::string name_;
    std::string pitchParam_, rangeParam_;
    juce::Slider fader_;
    HoldButton bendUp_, bendDown_;
    juce::TextButton range_;
};

}
