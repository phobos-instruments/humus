// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>

#include <juce_audio_devices/juce_audio_devices.h>

namespace hum {

class GuardedDeviceType final : public juce::AudioIODeviceType,
                                private juce::AudioIODeviceType::Listener {
public:
    struct Verdicts {
        juce::StringArray unsafe, refused;
    };
    using Probe = std::function<Verdicts(const juce::String& typeName,
                                         const juce::StringArray& drivers)>;

    GuardedDeviceType(std::unique_ptr<juce::AudioIODeviceType> inner, Probe probe);
    ~GuardedDeviceType() override;

    void scanForDevices() override;
    juce::StringArray getDeviceNames(bool wantInputNames = false) const override;
    int getDefaultDeviceIndex(bool forInput) const override;
    int getIndexOfDevice(juce::AudioIODevice* device, bool asInput) const override;
    bool hasSeparateInputsAndOutputs() const override;
    juce::AudioIODevice* createDevice(const juce::String& outputDeviceName,
                                      const juce::String& inputDeviceName) override;

    const juce::StringArray& hiddenDrivers() const { return verdicts_.unsafe; }

private:
    void audioDeviceListChanged() override { callDeviceChangeListeners(); }
    juce::StringArray allDrivers() const;

    std::unique_ptr<juce::AudioIODeviceType> inner_;
    Probe probe_;
    Verdicts verdicts_;
    juce::StringArray trusted_;
};

bool isGenericDriver(const juce::String& name);

void guardRiskyTypes(juce::OwnedArray<juce::AudioIODeviceType>& types,
                     const GuardedDeviceType::Probe& probe);

}
