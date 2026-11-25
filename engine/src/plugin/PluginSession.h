// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/host/DawMacroTarget.h"

namespace hum {

struct PluginSession {
    static constexpr int kMacros = 8;
    std::string documentPath;
    std::array<std::vector<DawMacroTarget>, kMacros> macros;
    bool playWhenStopped = true;
    std::string text;
};

juce::MemoryBlock encodeSession(const PluginSession& session);
std::optional<PluginSession> decodeSession(const void* data, int size);

}
