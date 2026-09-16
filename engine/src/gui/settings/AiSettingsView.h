// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/LookAndFeel.h"

namespace hum {

class AiSettingsView : public juce::Component {
public:
    AiSettingsView();

    void resized() override;

private:
    juce::Label title_, providerLabel_, endpointLabel_, modelLabel_, keyLabel_, hint_;
    juce::ComboBox providerCombo_;
    juce::TextEditor endpointEdit_, modelEdit_, keyEdit_;
    juce::TextButton testBtn_{"Test"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiSettingsView)
};

}
