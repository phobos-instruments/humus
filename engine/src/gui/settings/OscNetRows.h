// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/SettingsHost.h"

namespace hum {

juce::StringArray thisComputerAddresses();

class OscInputRow : public juce::Component {
public:
    explicit OscInputRow(SettingsHost* host);

    void resized() override;
    void refresh();

    juce::String statusForTest() const { return status_.getText(); }
    void typePortForTest(const juce::String& text) { port_.setText(text, false); commitPort(); }
    void clickForTest() { enable_.setToggleState(!enable_.getToggleState(), juce::dontSendNotification); toggle(); }

private:
    void toggle();
    void commitPort();

    SettingsHost* host_;
    juce::ToggleButton enable_;
    juce::Label portLabel_, status_;
    juce::TextEditor port_;
};

class OscOutputRow : public juce::Component {
public:
    OscOutputRow();

    void resized() override;

    void typeForTest(const juce::String& address, const juce::String& port) {
        address_.setText(address, false);
        port_.setText(port, false);
        commit();
    }

private:
    void commit();

    juce::Label title_, toLabel_, portLabel_;
    juce::TextEditor address_, port_;
};

}
