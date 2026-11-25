// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "gui/host/HostEnvironment.h"

namespace hum {

inline HostEnvironment appEnvironment() {
    HostEnvironment app;
    app.helperProgram = pathOf(juce::File::getSpecialLocation(juce::File::currentExecutableFile));
    return app;
}

inline HostEnvironment pluginEnvironment(const juce::File& installedApp) {
    HostEnvironment plugin;
    plugin.drivenByDaw = true;
    plugin.claimsMachineResources = false;
    plugin.sharesProcessWithOtherHosts = true;
    plugin.helperProgram = installedApp == juce::File() ? std::string() : pathOf(installedApp);
    return plugin;
}

}
