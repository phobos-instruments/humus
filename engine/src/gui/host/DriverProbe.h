// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <optional>

#include <juce_core/juce_core.h>

#include "gui/host/GuardedDeviceType.h"

namespace hum::driverprobe {

inline constexpr const char* kFlag = "--audio-driver-probe";
inline constexpr int kTimeoutMs = 10000;

enum class Verdict { Works, Refused, Unsafe, Unknown };

Verdict classify(bool started, bool finished, const juce::String& reply);

class VerdictStore {
public:
    explicit VerdictStore(juce::File file) : file_(std::move(file)) {}
    std::optional<Verdict> lookup(const juce::String& type, const juce::String& driver,
                                  const juce::String& key) const;
    void store(const juce::String& type, const juce::String& driver, const juce::String& key,
               Verdict verdict);

private:
    juce::File file_;
};

juce::String driverKey(const juce::String& type, const juce::String& driver);

juce::String openAndClose(const juce::String& type, const juce::String& driver);

GuardedDeviceType::Probe childProbe(juce::File executable, juce::File storeFile,
                                    int timeoutMs = kTimeoutMs);

}
