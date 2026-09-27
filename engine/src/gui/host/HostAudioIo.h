// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>

#include <juce_audio_devices/juce_audio_devices.h>

namespace hum {

class HostAudioIo {
public:
    virtual ~HostAudioIo() = default;
    virtual juce::String open(int inputs, int outputs, const juce::XmlElement* saved) = 0;
    virtual void close() = 0;
    virtual void attach(juce::AudioIODeviceCallback& callback) = 0;
    virtual void detach(juce::AudioIODeviceCallback& callback) = 0;
    virtual juce::AudioIODevice* current() = 0;
    virtual std::unique_ptr<juce::XmlElement> state() = 0;
    virtual juce::AudioDeviceManager::AudioDeviceSetup setup() = 0;
    virtual void applySetup(const juce::AudioDeviceManager::AudioDeviceSetup& setup) = 0;
    virtual juce::AudioDeviceManager& manager() = 0;
};

std::unique_ptr<HostAudioIo> deviceAudioIo();
std::unique_ptr<HostAudioIo> nullAudioIo();

}
