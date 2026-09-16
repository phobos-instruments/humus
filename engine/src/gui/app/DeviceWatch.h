// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <optional>

#include <juce_audio_devices/juce_audio_devices.h>

#include "gui/common/Localisation.h"

namespace hum {

struct DeviceChange {
    bool midi = false;
    juce::String title, message;
};

namespace devicewatch {

inline juce::StringArray midiNames() {
    juce::StringArray names;
    for (const auto& d : juce::MidiInput::getAvailableDevices()) names.addIfNotAlreadyThere(d.name);
    for (const auto& d : juce::MidiOutput::getAvailableDevices()) names.addIfNotAlreadyThere(d.name);
    return names;
}

inline juce::StringArray audioNames(juce::AudioDeviceManager& dm) {
    juce::StringArray names;
    for (auto* type : dm.getAvailableDeviceTypes()) {
        type->scanForDevices();
        for (const auto& n : type->getDeviceNames(false)) names.addIfNotAlreadyThere(n);
        for (const auto& n : type->getDeviceNames(true)) names.addIfNotAlreadyThere(n);
    }
    return names;
}

inline std::optional<DeviceChange> describe(const juce::StringArray& before,
                                            const juce::StringArray& after, bool midi) {
    juce::StringArray added, removed;
    for (const auto& n : after) if (!before.contains(n)) added.add(n);
    for (const auto& n : before) if (!after.contains(n)) removed.add(n);
    if (added.isEmpty() && removed.isEmpty()) return std::nullopt;
    DeviceChange c;
    c.midi = midi;
    const auto& names = added.isEmpty() ? removed : added;
    if (midi)
        c.title = added.isEmpty() ? tr("device-notice.midi-gone", "MIDI device disconnected")
                                  : tr("device-notice.midi-new", "New MIDI device");
    else
        c.title = added.isEmpty() ? tr("device-notice.audio-gone", "Audio device disconnected")
                                  : tr("device-notice.audio-new", "New audio device");
    c.message = names.joinIntoString(", ");
    return c;
}

}

class DeviceWatch : private juce::ChangeListener {
public:
    explicit DeviceWatch(juce::AudioDeviceManager& dm) : dm_(dm) {
        midi_ = devicewatch::midiNames();
        dm_.addChangeListener(this);
        midiConn_ = juce::MidiDeviceListConnection::make([this] { midiListChanged(); });
    }
    ~DeviceWatch() override { dm_.removeChangeListener(this); }

    std::function<void(const DeviceChange&)> onChange;

private:
    void midiListChanged() {
        const auto now = devicewatch::midiNames();
        if (const auto c = devicewatch::describe(midi_, now, true); c && onChange) onChange(*c);
        midi_ = now;
    }
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        const auto now = devicewatch::audioNames(dm_);
        if (audioPrimed_)
            if (const auto c = devicewatch::describe(audio_, now, false); c && onChange) onChange(*c);
        audio_ = now;
        audioPrimed_ = true;
    }

    juce::AudioDeviceManager& dm_;
    juce::MidiDeviceListConnection midiConn_;
    juce::StringArray midi_, audio_;
    bool audioPrimed_ = false;
};

}
