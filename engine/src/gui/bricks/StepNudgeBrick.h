// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostPattern.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/Mappable.h"
#include "gui/editor/OrganismEditor.h"
#include "hum/PatternMatrix.h"

namespace hum {

class StepNudgeBrick : public OrganismEditor {
public:
    StepNudgeBrick(BrickHost& host, std::string name, std::string param)
        : host_(host), name_(std::move(name)), param_(std::move(param)) {
        earlier_.setButtonText(juce::String::fromUTF8("\xe2\x97\x80"));
        later_.setButtonText(juce::String::fromUTF8("\xe2\x96\xb6"));
        earlier_.setTooltip(tr("step-nudge.earlier", "Nudge the pattern a step earlier"));
        later_.setTooltip(tr("step-nudge.later", "Nudge the pattern a step later"));
        earlier_.onClick = [this] { nudge(-1); };
        later_.onClick = [this] { nudge(1); };
        earlier_.onRightClick = later_.onRightClick = [this](juce::Point<int> at) { showMenu(at); };
        value_.setJustificationType(juce::Justification::centred);
        value_.setFont(juce::FontOptions(12.0f));
        value_.setTooltip(tr("step-nudge.value", "Steps off the grid - double-click resets"));
        value_.addMouseListener(this, false);
        addAndMakeVisible(earlier_);
        addAndMakeVisible(value_);
        addAndMakeVisible(later_);
        refreshAutomatedValues();
    }

    std::function<void()> onNudged;

    int current() const { return (int) std::lround(host_.liveParamValue(name_, param_)); }

    void nudge(int by) { set(wrappedNudge(current(), by, steps())); }

    void set(int to) {
        if (to == current()) return;
        host_.pushUndo();
        host_.setParam(name_, param_, (double) to);
        refreshAutomatedValues();
        if (onNudged) onNudged();
    }

    void reloadValues() override { refreshAutomatedValues(); }
    void refreshAutomatedValues() override {
        const int v = current();
        const auto text = v > 0 ? "+" + juce::String(v) : juce::String(v);
        if (value_.getText() != text) value_.setText(text, juce::dontSendNotification);
        const auto ink = v != 0 ? Palette::accent : Palette::textDim;
        if (value_.findColour(juce::Label::textColourId) != ink)
            value_.setColour(juce::Label::textColourId, ink);
    }
    int preferredContentWidth() const override { return 96; }
    int preferredContentHeight(int) const override { return 22; }

    void resized() override {
        auto r = getLocalBounds();
        const int arrow = r.getWidth() / 3;
        earlier_.setBounds(r.removeFromLeft(arrow).reduced(1, 0));
        later_.setBounds(r.removeFromRight(arrow).reduced(1, 0));
        value_.setBounds(r);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.eventComponent == &value_ && e.mods.isPopupMenu()) showMenu(e.getScreenPosition());
    }
    void mouseDoubleClick(const juce::MouseEvent& e) override {
        if (e.eventComponent == &value_) set(0);
    }

private:
    int steps() const { return (int) host_.patterns().basslineSteps(name_).size(); }

    void showMenu(juce::Point<int> at) {
        showAutomateMenu(host_, name_, param_, at, [safe = juce::Component::SafePointer<StepNudgeBrick>(this)] {
            if (safe != nullptr) safe->refreshAutomatedValues();
        });
    }

    BrickHost& host_;
    std::string name_;
    std::string param_;
    Mappable<juce::TextButton> earlier_, later_;
    juce::Label value_;
};

}
