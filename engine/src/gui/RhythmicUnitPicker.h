#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"

namespace hum {

class RhythmicUnitPicker : public juce::Component {
public:
    RhythmicUnitPicker(EngineHost& host, std::string organism,
                       std::string multiplierParam, std::string unitParam)
        : host_(host), name_(std::move(organism)),
          multParam_(std::move(multiplierParam)), unitParam_(std::move(unitParam)) {
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

        static const char* units[] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"};
        for (auto* u : units) unitBox_.addItem(u, unitBox_.getNumItems() + 1);
        unitBox_.setEditableText(true);
        unitBox_.setTextWhenNothingSelected("1/16");
        unitBox_.onChange = [this] { commitUnit(); };

        refresh();
    }

    void refresh() {
        double m = host_.liveParamValue(name_, multParam_);
        multBox_.setText(juce::String(m), juce::dontSendNotification);
        juce::String u = juce::String(host_.liveParamText(name_, unitParam_));
        if (u.isEmpty()) u = "1/16";
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
        double m = multBox_.getText().getDoubleValue();
        if (m < 0.001) m = 0.001;
        host_.setParam(name_, multParam_, m);
    }

    void commitUnit() {
        host_.setParamText(name_, unitParam_, unitBox_.getText().toStdString());
    }

    EngineHost& host_;
    std::string name_, multParam_, unitParam_;
    juce::TextEditor multBox_;
    juce::Label xLabel_;
    juce::ComboBox unitBox_;
};

}
