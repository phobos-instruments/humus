// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <vector>

#include "core/plugins/HostedPlugin.h"
#include "core/app/UiWatchdog.h"
#include "gui/host/PluginsHost.h"

namespace hum {

inline bool mirrorPluginParams(PluginsHost& host, const std::string& name, HostedPlugin& plugin) {
    if (auto* w = UiWatchdog::active()) w->noteActivePlugin(plugin.classRaw());
    if (!plugin.paramsPushed()) return false;
    const auto& pluginParams = plugin.instance()->getParameters();
    const auto& ourParams = plugin.params.all();
    if (ourParams.empty()) return false;

    std::vector<double> hostValues;
    host.batchLiveParamValues(name, ourParams, hostValues);

    bool wrote = false;
    for (size_t i = 0; i < ourParams.size(); ++i) {
        const auto& p = ourParams[i];
        if (p.index < 0 || p.index >= (int) pluginParams.size()) continue;
        const double pluginValue = (double) pluginParams[(size_t) p.index]->getValue();
        if (std::abs(pluginValue - hostValues[i]) > 1e-4) {
            host.setParam(name, p.name, pluginValue);
            wrote = true;
        }
    }
    return wrote;
}

}
