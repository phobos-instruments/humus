// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <memory>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiControl.h"
#include "gui/app/AppSettings.h"
#include "gui/host/SettingsHost.h"
#include "gui/host/MidiState.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"
#include "gui/settings/OscSerialRow.h"

namespace hum {

class MidiSettingsView : public juce::Component, private juce::Timer {
public:
    explicit MidiSettingsView(SettingsHost* host);

    void resized() override;

private:
    void applyToHost();

    void rebuildDeviceLists();

    struct MapRow {
        std::unique_ptr<juce::Label> text;
        std::unique_ptr<juce::TextButton> remove;
        std::unique_ptr<juce::ToggleButton> latching, ownAction;
    };

    void timerCallback() override;

    void rebuildMappings();

    void addModifierRow(const MidiModifier& m);

    void layoutMappingRows();

    struct PortRow {
        juce::Label label;
        juce::ComboBox combo;
    };

    SettingsHost* host_;
    juce::Label title_, inputsLabel_, outputLabel_, hint_, mapLabel_, oscPortLabel_,
                syncLabel_;
    juce::ComboBox syncCombo_;
    juce::TextButton btMidi_;
    std::array<PortRow, MidiState::kPorts> inRows_, outRows_;
    std::vector<juce::MidiDeviceInfo> inputs_, outputs_;
    juce::ToggleButton oscEnable_;
    juce::TextEditor oscPort_;
    OscSerialRow oscSerial_{host_};
    juce::ToggleButton padEnable_;
    juce::Label padStatus_;
    juce::Label fineLabel_;
    juce::Slider fine_;
    juce::Viewport mapViewport_;
    juce::Component mapRows_;
    std::vector<MapRow> mapRowWidgets_;
    size_t lastMapCount_ = ~(size_t) 0;
    juce::MidiDeviceListConnection listConn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSettingsView)
};

}
