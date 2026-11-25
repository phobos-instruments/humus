// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/common/Localisation.h"

namespace hum::i18n {

inline std::vector<std::pair<juce::String, juce::String>> juceStrings() {
    return {
        {"Output:", tr("juce.output", "Output:")},
        {"Input:", tr("juce.input", "Input:")},
        {"none", tr("juce.none", "none")},
        {"Test", tr("juce.test", "Test")},
        {"Plays a test tone", tr("juce.plays-a-test-tone", "Plays a test tone")},
        {"Device:", tr("juce.device", "Device:")},
        {"Audio device type:", tr("juce.audio-device-type", "Audio device type:")},
        {"Sample rate:", tr("juce.sample-rate", "Sample rate:")},
        {"Audio buffer size:", tr("juce.audio-buffer-size", "Audio buffer size:")},
        {"Active output channels:", tr("juce.active-output-channels", "Active output channels:")},
        {"Active input channels:", tr("juce.active-input-channels", "Active input channels:")},
        {"(no audio output channels found)", tr("juce.no-output-channels", "(no audio output channels found)")},
        {"(no audio input channels found)", tr("juce.no-input-channels", "(no audio input channels found)")},
        {"Control Panel", tr("juce.control-panel", "Control Panel")},
        {"Opens the device's own control panel", tr("juce.opens-control-panel", "Opens the device's own control panel")},
        {"Reset Device", tr("juce.reset-device", "Reset Device")},
        {"Resets the audio interface - sometimes needed after changing a device's properties in its custom control panel",
         tr("juce.resets-device",
            "Resets the audio interface - sometimes needed after changing a device's properties in its custom control panel")},
        {"Show advanced settings...", tr("juce.show-advanced", "Show advanced settings...")},
        {"Error when trying to open audio device!", tr("juce.open-error", "Error when trying to open audio device!")},
        {"Active MIDI inputs:", tr("juce.active-midi-inputs", "Active MIDI inputs:")},
        {"No MIDI inputs available", tr("juce.no-midi-inputs", "No MIDI inputs available")},
        {"MIDI Output:", tr("juce.midi-output", "MIDI Output:")},
        {"Bluetooth MIDI", tr("juce.bluetooth-midi", "Bluetooth MIDI")},
        {"Scan for bluetooth MIDI devices", tr("juce.scan-bluetooth", "Scan for bluetooth MIDI devices")},
    };
}

inline juce::String juceMappingText() {
    juce::String text;
    for (const auto& [english, shown] : juceStrings())
        if (shown != english && !shown.containsChar('"'))
            text << "\"" << english << "\" = \"" << shown << "\"\n";
    return text;
}

inline void installJuceStrings() {
    const auto text = juceMappingText();
    juce::LocalisedStrings::setCurrentMappings(text.isEmpty() ? nullptr : new juce::LocalisedStrings(text, false));
}

}
