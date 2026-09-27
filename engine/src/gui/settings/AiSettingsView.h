// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/LookAndFeel.h"

namespace hum {

class AiSettingsView : public juce::Component {
public:
    AiSettingsView();

    void probe(bool announce);
    juce::ComboBox& modelBox() { return modelCombo_; }
    juce::ComboBox& providerBox() { return providerCombo_; }
    juce::String hintText() const { return hint_.getText(); }

    void resized() override;

private:
    juce::String endpointBase() const;
    void fillModels(const juce::StringArray& names);
    void onProviderChanged();

    juce::Label title_, providerLabel_, endpointLabel_, modelLabel_, keyLabel_, hint_;
    juce::ComboBox providerCombo_, modelCombo_;
    juce::TextEditor endpointEdit_, keyEdit_;
    juce::TextButton testBtn_{"Test"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiSettingsView)
};

}
