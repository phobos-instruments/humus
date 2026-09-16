// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamUnit.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class RangePrompt : public juce::Component {
public:
    static void show(juce::Rectangle<int> anchor, const juce::String& title, Unit unit,
                     double lo, double hi, std::function<void(double, double)> onPick) {
        auto owned = std::make_unique<RangePrompt>(title, unit, lo, hi, std::move(onPick));
        auto* raw = owned.get();
        auto& box = juce::CallOutBox::launchAsynchronously(std::move(owned), anchor, nullptr);
        raw->box_ = &box;
    }

    RangePrompt(const juce::String& title, Unit unit, double lo, double hi,
                std::function<void(double, double)> onPick)
        : unit_(unit), lo_(lo), hi_(hi), onPick_(std::move(onPick)) {
        title_.setText(title, juce::dontSendNotification);
        title_.setColour(juce::Label::textColourId, Palette::textDim);
        title_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(title_);

        auto field = [this](juce::TextEditor& e, double v) {
            e.setText(text(v), juce::dontSendNotification);
            e.setJustification(juce::Justification::centred);
            e.onReturnKey = [this] { commit(); };
            addAndMakeVisible(e);
        };
        field(min_, lo);
        field(max_, hi);
        fromLabel_.setText("from", juce::dontSendNotification);
        toLabel_.setText("to", juce::dontSendNotification);
        const juce::String sfx = unitSuffix(unit);
        minUnit_.setText(sfx, juce::dontSendNotification);
        maxUnit_.setText(sfx, juce::dontSendNotification);
        for (auto* l : {&minUnit_, &maxUnit_}) {
            l->setColour(juce::Label::textColourId, Palette::textDim);
            l->setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(*l);
        }
        for (auto* l : {&fromLabel_, &toLabel_}) {
            l->setColour(juce::Label::textColourId, Palette::textDim);
            l->setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(*l);
        }
        okBtn_.onClick = [this] { commit(); };
        addAndMakeVisible(okBtn_);
        setSize(304, 96);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        title_.setBounds(r.removeFromTop(18));
        r.removeFromTop(4);
        auto row = r.removeFromTop(24);
        fromLabel_.setBounds(row.removeFromLeft(40));
        row.removeFromLeft(4);
        min_.setBounds(row.removeFromLeft(64));
        minUnit_.setBounds(row.removeFromLeft(38).reduced(4, 0));
        toLabel_.setBounds(row.removeFromLeft(24));
        row.removeFromLeft(4);
        max_.setBounds(row.removeFromLeft(64));
        maxUnit_.setBounds(row.removeFromLeft(38).reduced(4, 0));
        r.removeFromTop(8);
        okBtn_.setBounds(r.removeFromTop(24).removeFromRight(72));
    }

private:
    juce::String text(double v) const { return unitPlain(unit_, v, lo_, hi_); }
    double value(const juce::String& t) const { return unitPlainParse(unit_, t, lo_, hi_); }

    void commit() {
        const double a = value(min_.getText()), b = value(max_.getText());
        auto cb = onPick_;
        if (box_ != nullptr) box_->dismiss();
        if (cb) cb(a, b);
    }

    juce::Label title_, fromLabel_, toLabel_, minUnit_, maxUnit_;
    juce::TextEditor min_, max_;
    juce::TextButton okBtn_{"OK"};
    Unit unit_;
    double lo_, hi_;
    std::function<void(double, double)> onPick_;
    juce::CallOutBox* box_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RangePrompt)
};

}
