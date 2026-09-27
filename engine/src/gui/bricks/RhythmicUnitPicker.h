// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/inputs/NumberInputs.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class RhythmicUnitPicker : public juce::Component {
public:
    RhythmicUnitPicker(BrickHost& host, std::string organism,
                       std::string multiplierParam, std::string unitParam)
        : unit_(host, std::move(organism), std::move(multiplierParam), std::move(unitParam)) {
        addAndMakeVisible(multBox_);
        addAndMakeVisible(xLabel_);
        addAndMakeVisible(unitBox_);

        xLabel_.setText("x", juce::dontSendNotification);
        xLabel_.setJustificationType(juce::Justification::centred);
        xLabel_.setColour(juce::Label::textColourId, Palette::textDim);
        xLabel_.setFont(juce::FontOptions(11.0f));

        multBox_.setJustification(juce::Justification::centred);
        multBox_.setInputRestrictions(8, "0123456789.");
        multBox_.onReturnKey = [this] { commitMultiplier(); };
        multBox_.onFocusLost = [this] { commitMultiplier(); };

        for (const auto& u : input::RhythmicUnit::units()) unitBox_.addItem(u, unitBox_.getNumItems() + 1);
        unitBox_.setEditableText(true);
        unitBox_.setTextWhenNothingSelected(input::RhythmicUnit::kDefaultUnit);
        unitBox_.onChange = [this] { commitUnit(); };

        refresh();
    }

    void refresh() {
        multBox_.setText(juce::String(unit_.multiplier()), juce::dontSendNotification);
        const juce::String u(unit_.unit());
        int id = 0;
        for (int i = 0; i < unitBox_.getNumItems(); ++i)
            if (unitBox_.getItemText(i) == u) { id = unitBox_.getItemId(i); break; }
        if (id > 0) unitBox_.setSelectedId(id, juce::dontSendNotification);
        else unitBox_.setText(u, juce::dontSendNotification);
    }

    void resized() override {
        auto r = getLocalBounds();
        multBox_.setBounds(r.removeFromLeft(40));
        r.removeFromLeft(2);
        xLabel_.setBounds(r.removeFromLeft(12));
        r.removeFromLeft(2);
        unitBox_.setBounds(r);
    }

private:
    void commitMultiplier() {
        unit_.commitMultiplier(multBox_.getText().getDoubleValue());
    }

    void commitUnit() {
        unit_.commitUnit(unitBox_.getText().toStdString());
    }

    input::RhythmicUnit unit_;
    juce::TextEditor multBox_;
    juce::Label xLabel_;
    juce::ComboBox unitBox_;
};

}
