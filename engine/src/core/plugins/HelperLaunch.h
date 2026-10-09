// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstddef>

#include <juce_core/juce_core.h>

#include "core/plugins/BridgeClient.h"

namespace hum::launch {

inline const std::array<const char*, 6>& helperFlags() {
    static const std::array<const char*, 6> flags = {
        "--ai-ping", "--tuning-probe", "--scan-enumerate-out",
        "--scan-plugin-out", "--scan-plugin", "--audio-driver-probe"};
    return flags;
}

inline juce::String bridgePrefix() {
    return juce::String("--") + BridgeClient::kUid + ":";
}

inline bool isHelper(const juce::StringArray& args) {
    for (const char* flag : helperFlags())
        if (args.contains(flag)) return true;
    for (const auto& a : args)
        if (a.startsWith(bridgePrefix())) return true;
    return false;
}

}
