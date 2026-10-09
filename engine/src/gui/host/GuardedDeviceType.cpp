// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/GuardedDeviceType.h"

#include <utility>

namespace hum {

namespace {

const juce::StringArray& riskyTypeNames() {
    static const juce::StringArray names{"ASIO"};
    return names;
}

juce::StringArray without(const juce::StringArray& names, const juce::StringArray& hidden) {
    juce::StringArray out;
    for (const auto& n : names)
        if (!hidden.contains(n)) out.add(n);
    return out;
}

}

bool isGenericDriver(const juce::String& name) {
    for (const auto* generic : {"asio4all", "flexasio", "asio2wasapi", "generic low latency"})
        if (name.containsIgnoreCase(generic)) return true;
    return false;
}

GuardedDeviceType::GuardedDeviceType(std::unique_ptr<juce::AudioIODeviceType> inner, Probe probe)
    : juce::AudioIODeviceType(inner->getTypeName()), inner_(std::move(inner)),
      probe_(std::move(probe)) {
    inner_->addListener(this);
}

GuardedDeviceType::~GuardedDeviceType() { inner_->removeListener(this); }

juce::StringArray GuardedDeviceType::allDrivers() const {
    auto names = inner_->getDeviceNames(false);
    names.mergeArray(inner_->getDeviceNames(true));
    return names;
}

void GuardedDeviceType::scanForDevices() {
    inner_->scanForDevices();
    const auto unproven = without(allDrivers(), trusted_);
    verdicts_ = probe_ ? probe_(getTypeName(), unproven) : Verdicts{};
}

juce::StringArray GuardedDeviceType::getDeviceNames(bool wantInputNames) const {
    return without(inner_->getDeviceNames(wantInputNames), verdicts_.unsafe);
}

int GuardedDeviceType::getDefaultDeviceIndex(bool forInput) const {
    const auto offered = getDeviceNames(forInput);
    if (offered.isEmpty()) return -1;
    const auto works = [this](const juce::String& n) { return !verdicts_.refused.contains(n); };
    for (int i = 0; i < offered.size(); ++i)
        if (works(offered[i]) && !isGenericDriver(offered[i])) return i;
    for (int i = 0; i < offered.size(); ++i)
        if (works(offered[i])) return i;
    const auto innerNames = inner_->getDeviceNames(forInput);
    const int innerDefault = inner_->getDefaultDeviceIndex(forInput);
    if (juce::isPositiveAndBelow(innerDefault, innerNames.size()))
        if (const int at = offered.indexOf(innerNames[innerDefault]); at >= 0) return at;
    return 0;
}

int GuardedDeviceType::getIndexOfDevice(juce::AudioIODevice* device, bool asInput) const {
    return device != nullptr ? getDeviceNames(asInput).indexOf(device->getName()) : -1;
}

bool GuardedDeviceType::hasSeparateInputsAndOutputs() const {
    return inner_->hasSeparateInputsAndOutputs();
}

juce::AudioIODevice* GuardedDeviceType::createDevice(const juce::String& outputDeviceName,
                                                     const juce::String& inputDeviceName) {
    if (verdicts_.unsafe.contains(outputDeviceName) || verdicts_.unsafe.contains(inputDeviceName))
        return nullptr;
    auto* device = inner_->createDevice(outputDeviceName, inputDeviceName);
    if (device != nullptr) {
        trusted_.addIfNotAlreadyThere(outputDeviceName);
        trusted_.addIfNotAlreadyThere(inputDeviceName);
    }
    return device;
}

void guardRiskyTypes(juce::OwnedArray<juce::AudioIODeviceType>& types,
                     const GuardedDeviceType::Probe& probe) {
    for (int i = 0; i < types.size(); ++i) {
        if (!riskyTypeNames().contains(types[i]->getTypeName())) continue;
        std::unique_ptr<juce::AudioIODeviceType> inner(types.removeAndReturn(i));
        types.insert(i, new GuardedDeviceType(std::move(inner), probe));
    }
}

}
