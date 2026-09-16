// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_core/juce_core.h>

#include "gui/app/AppSettings.h"

namespace hum::bridgePolicy {

inline bool sandboxEnabled() {
#if JUCE_LINUX
    return AppSettings::instance().getInt("plugins.bridge.enabled", 1) != 0;
#else
    return false;
#endif
}
inline void setSandboxEnabled(bool on) {
    AppSettings::instance().set("plugins.bridge.enabled", on ? 1 : 0);
}

inline juce::StringArray inProcessList() {
    juce::StringArray a;
    a.addTokens(AppSettings::instance().getString("plugins.inprocess", ""), "\n", "");
    a.removeEmptyStrings();
    return a;
}
inline bool isInProcess(const std::string& classRaw) {
    return inProcessList().contains(juce::String(classRaw));
}
inline void setInProcess(const std::string& classRaw, bool inProcess) {
    auto a = inProcessList();
    if (inProcess) a.addIfNotAlreadyThere(juce::String(classRaw));
    else           a.removeString(juce::String(classRaw));
    AppSettings::instance().set("plugins.inprocess", a.joinIntoString("\n"));
}

inline bool shouldBridge(const std::string& classRaw) {
    return sandboxEnabled() && !isInProcess(classRaw);
}

}
