// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/editor/decks/DeckModels.h"
#include "gui/host/EngineHostDeck.h"
#include "gui/editor/FineDrag.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class HoldButton : public juce::TextButton {
public:
    using juce::TextButton::TextButton;
    std::function<void(bool)> onHold;
    void mouseDown(const juce::MouseEvent& e) override {
        juce::TextButton::mouseDown(e);
        if (!e.mods.isPopupMenu() && onHold) onHold(true);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        juce::TextButton::mouseUp(e);
        if (!e.mods.isPopupMenu() && onHold) onHold(false);
    }
};

class PitchFader : public juce::Component, private juce::Timer {
public:
    static constexpr double kBend = decks::PitchModel::kBend;

    PitchFader(BrickHost& host, std::string organism, const Bindings& bound)
        : host_(host), name_(organism),
          pitch_(host, host.decks(), std::move(organism), bound(bind::kPitch),
                 bound(bind::kPitchRange), bound(bind::kBpm)),
          upParam_(bound(bind::kBendUp)), downParam_(bound(bind::kBendDown)),
          rangeParam_(bound(bind::kPitchRange)),
          fader_(juce::Slider::LinearVertical, juce::Slider::NoTextBox),
          bendUp_("+"), bendDown_("-"), range_("8%") {
        const double r = pitch_.range();
        fader_.setRange(-r, r, 0.0);
        applyFineCrawl(fader_);
        fader_.setValue(pitch_.pitch(), juce::dontSendNotification);
        fader_.setDoubleClickReturnValue(true, 0.0);
        fader_.onValueChange = [this] { pitch_.edit(fader_.getValue()); repaint(); };
        fader_.onDragStart = [this] { pitch_.beginDrag(); };
        fader_.onDragEnd = [this] { pitch_.endDrag(); };
        addAndMakeVisible(fader_);

        dress(bendUp_, upParam_);
        dress(bendDown_, downParam_);
        dress(range_, rangeParam_);
        bendUp_.setTooltip(tr("pitch-fader.nudge-tempo-up-hold", "Nudge tempo up (hold)"));
        bendDown_.setTooltip(tr("pitch-fader.nudge-tempo-down-hold", "Nudge tempo down (hold)"));
        range_.setTooltip(tr("pitch-fader.pitch-range", "Pitch range"));
        bendUp_.onHold   = [this](bool d) { host_.setParam(name_, upParam_, d ? 1.0 : 0.0); };
        bendDown_.onHold = [this](bool d) { host_.setParam(name_, downParam_, d ? 1.0 : 0.0); };
        range_.onClick = [this] { pitch_.cycleRange(); };
        startTimerHz(20);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(2);
        const int bw = juce::jmin(r.getWidth(), 60);
        auto mid = [&](juce::Rectangle<int> row) { return row.withSizeKeepingCentre(bw, row.getHeight()); };
        bendUp_.setBounds(mid(r.removeFromTop(22)));
        range_.setBounds(mid(r.removeFromBottom(20)));
        bendDown_.setBounds(mid(r.removeFromBottom(22)));
        r.removeFromTop(30);
        fader_.setBounds(r.withSizeKeepingCentre(juce::jmin(r.getWidth(), 44), r.getHeight()));
    }

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String(pitch_.pitchText()),
                   getLocalBounds().withTop(24).withHeight(14), juce::Justification::centred);
        const auto driven = pitch_.drivenText();
        if (driven.empty()) return;
        g.setColour(Palette::accent);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(juce::String(driven),
                   getLocalBounds().withTop(38).withHeight(14), juce::Justification::centred);
    }

private:
    template <typename Button>
    void dress(Button& b, const std::string& param) {
        const auto* cm = host_.model().byName(name_);
        const auto family = familyOf(cm != nullptr ? cm->classRaw : std::string());
        b.onRestyle = [&b, family] {
            b.setColour(juce::TextButton::buttonColourId, Palette::panelLight);
            b.setColour(juce::TextButton::buttonOnColourId, Palette::familyAccent(family));
            b.setColour(juce::TextButton::textColourOffId, Palette::text);
            b.setColour(juce::TextButton::textColourOnId, Palette::background);
        };
        b.onRestyle();
        if (!param.empty()) {
            b.onRightClick = [this, param](juce::Point<int> at) {
                showAutomateMenu(host_, name_, param, at, [this] { repaint(); });
            };
            learned_.push_back({[&b](bool controlled, bool locked, bool lit) {
                                    b.setMarks(controlled, locked);
                                    b.setLit(lit);
                                },
                                param});
        }
        addAndMakeVisible(b);
    }

    struct Learned { std::function<void(bool, bool, bool)> mark; std::string param; };

    void timerCallback() override {
        const double r = pitch_.range();
        if (std::abs(fader_.getMaximum() - r) > 1e-6) fader_.setRange(-r, r, 0.0);
        const double v = pitch_.pitch();
        if (std::abs(fader_.getValue() - v) > 1e-6) fader_.setValue(v, juce::dontSendNotification);
        range_.setButtonText(juce::String(pitch_.rangeText()));
        for (const auto& [mark, param] : learned_)
            mark(paramIsControlled(host_, name_, param), host_.rollLocked(name_, param),
                 host_.firedRecently(name_, param));
        repaint();
    }

    BrickHost& host_;
    std::string name_;
    decks::PitchModel pitch_;
    std::string upParam_, downParam_, rangeParam_;
    juce::Slider fader_;
    Mappable<HoldButton> bendUp_, bendDown_;
    Mappable<juce::TextButton> range_;
    std::vector<Learned> learned_;
};

}
