// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_core/juce_core.h>

#include "core/packs/PackLoader.h"
#include "core/packs/PackRegistry.h"
#include "core/plugins/PluginHost.h"
#include "core/plugins/PluginListStore.h"
#include "gui/app/AppSettings.h"

namespace hum {

inline void loadUserExtensions() {
    PackRegistry::instance().loadBuiltinPacks();
    PackLoader::instance().loadInstalledPacks();
    PluginHost::instance().restoreKnownListFromXml(pluginListStore::load());
    juce::StringArray disabled;
    disabled.addTokens(AppSettings::instance().getString("packs.disabled"), ",", "");
    for (auto& id : disabled)
        if (id.isNotEmpty()) PackRegistry::instance().setPackEnabled(id.toStdString(), false);
}

}
