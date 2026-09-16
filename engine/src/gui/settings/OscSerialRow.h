// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/AppSettings.h"
#include "gui/host/SettingsHost.h"
#include "gui/host/MidiState.h"
#include "gui/host/OscHost.h"
#include "gui/common/Localisation.h"
#include "hum/SerialPort.h"

namespace hum {

class OscSerialRow : public juce::Component {
public:
    explicit OscSerialRow(SettingsHost* host) : host_(host) {
        auto& s = AppSettings::instance();
        enable_.setButtonText(tr("midi-settings.osc-over-serial", "OSC over serial (SLIP)"));
        enable_.setToggleState(s.getInt("osc.serial.enabled", 0) != 0, juce::dontSendNotification);
        enable_.onClick = [this] { fillPorts(); apply(); };
        addAndMakeVisible(enable_);
        fillPorts();
        port_.onChange = [this] { apply(); };
        addAndMakeVisible(port_);
        int id = 1;
        for (const int b : serial::standardBauds()) baud_.addItem(juce::String(b), id++);
        const int baud = s.getInt("osc.serial.baud", 115200);
        for (size_t i = 0; i < serial::standardBauds().size(); ++i)
            if (serial::standardBauds()[i] == baud) baud_.setSelectedId((int) i + 1, juce::dontSendNotification);
        if (baud_.getSelectedId() == 0) baud_.setSelectedId(9, juce::dontSendNotification);
        baud_.onChange = [this] { apply(); };
        addAndMakeVisible(baud_);
    }

    void resized() override {
        auto r = getLocalBounds();
        enable_.setBounds(r.removeFromLeft(190));
        port_.setBounds(r.removeFromLeft(200).reduced(0, 1));
        r.removeFromLeft(8);
        baud_.setBounds(r.removeFromLeft(100).reduced(0, 1));
    }

private:
    void fillPorts() {
        const auto want = AppSettings::instance().getString("osc.serial.port").toStdString();
        port_.clear(juce::dontSendNotification);
        port_.addItem(tr("midi-settings.serial-auto", "Auto - first USB serial"), 1);
        devices_ = serial::listDevices();
        int selected = 1;
        for (size_t i = 0; i < devices_.size(); ++i) {
            const auto slash = devices_[i].rfind('/');
            port_.addItem(juce::String(slash == std::string::npos ? devices_[i] : devices_[i].substr(slash + 1)),
                          (int) i + 2);
            if (devices_[i] == want) selected = (int) i + 2;
        }
        port_.setSelectedId(selected, juce::dontSendNotification);
    }

    void apply() {
        auto& s = AppSettings::instance();
        const int id = port_.getSelectedId();
        const std::string path = id >= 2 && (size_t) (id - 2) < devices_.size() ? devices_[(size_t) (id - 2)] : "";
        const int baud = baud_.getSelectedId() > 0 ? serial::standardBauds()[(size_t) baud_.getSelectedId() - 1] : 115200;
        s.set("osc.serial.enabled", enable_.getToggleState() ? 1 : 0);
        s.set("osc.serial.port", juce::String(path));
        s.set("osc.serial.baud", baud);
        if (host_ != nullptr) host_->osc().applySerialSettings();
    }

    SettingsHost* host_;
    juce::ToggleButton enable_;
    juce::ComboBox port_, baud_;
    std::vector<std::string> devices_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscSerialRow)
};

}
