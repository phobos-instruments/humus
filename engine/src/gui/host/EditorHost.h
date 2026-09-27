// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "gui/host/BrickHost.h"

namespace juce {
class AudioDeviceManager;
}

namespace hum {

class HostedPlugin;

class EditorHost : public virtual BrickHost {
public:
    ~EditorHost() override = default;

    virtual juce::AudioDeviceManager& audioDevices() = 0;

    virtual HostedPlugin* hostedPluginFor(const std::string& name) = 0;
};

}
