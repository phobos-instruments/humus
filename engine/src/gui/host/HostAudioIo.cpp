// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/HostAudioIo.h"

namespace hum {

namespace {

class DeviceAudioIo final : public HostAudioIo {
public:
    juce::String open(int inputs, int outputs, const juce::XmlElement* saved) override {
        return devices_.initialise(inputs, outputs, saved, true);
    }
    void close() override { devices_.closeAudioDevice(); }
    void attach(juce::AudioIODeviceCallback& callback) override { devices_.addAudioCallback(&callback); }
    void detach(juce::AudioIODeviceCallback& callback) override { devices_.removeAudioCallback(&callback); }
    juce::AudioIODevice* current() override { return devices_.getCurrentAudioDevice(); }
    std::unique_ptr<juce::XmlElement> state() override { return devices_.createStateXml(); }
    juce::AudioDeviceManager::AudioDeviceSetup setup() override { return devices_.getAudioDeviceSetup(); }
    void applySetup(const juce::AudioDeviceManager::AudioDeviceSetup& setup) override {
        devices_.setAudioDeviceSetup(setup, true);
    }
    juce::AudioDeviceManager& manager() override { return devices_; }

private:
    juce::AudioDeviceManager devices_;
};

class NullAudioIo final : public HostAudioIo {
public:
    juce::String open(int, int, const juce::XmlElement*) override { return "no audio device in this host"; }
    void close() override {}
    void attach(juce::AudioIODeviceCallback&) override {}
    void detach(juce::AudioIODeviceCallback&) override {}
    juce::AudioIODevice* current() override { return nullptr; }
    std::unique_ptr<juce::XmlElement> state() override { return nullptr; }
    juce::AudioDeviceManager::AudioDeviceSetup setup() override { return {}; }
    void applySetup(const juce::AudioDeviceManager::AudioDeviceSetup&) override {}
    juce::AudioDeviceManager& manager() override { return idle_; }

private:
    juce::AudioDeviceManager idle_;
};

}

std::unique_ptr<HostAudioIo> deviceAudioIo() { return std::make_unique<DeviceAudioIo>(); }

std::unique_ptr<HostAudioIo> nullAudioIo() { return std::make_unique<NullAudioIo>(); }

}
