// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_core/juce_core.h>

#include "core/plugins/BridgeRing.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginHost.h"
#include "gui/plugins/PluginBridgePolicy.h"
#include "gui/plugins/PluginEditorTeardown.h"
#include "gui/plugins/clap/ClapFormat.h"

namespace hum {

inline void setUpPluginHosting(const juce::File& bridgeProgram) {
    claphost::enableClapHosting();
    BridgeRing::sweepStale();
    if (bridgeProgram.existsAsFile()) PluginHost::instance().setBridgeExe(bridgeProgram);
    PluginHost::instance().setBridgePolicy(
        [](const std::string& classRaw) { return bridgePolicy::shouldBridge(classRaw); });
    HostedPlugin::shouldLeakInstance = [](const std::string& classRaw) {
        return matchesPluginList(classRaw, "plugins.leakInstance", "Synplant");
    };
    HostedPlugin::disposeGuard = [](const std::string& classRaw) -> std::shared_ptr<void> {
        return std::make_shared<EditorOpGuard>(classRaw, "plugins.leakInstance", "disposing its instance");
    };
}

}
