// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin/HumusProcessor.h"

#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace {

void keepModuleLoadedUntilProcessExit() {
    static const bool pinned = [] {
        HMODULE self = nullptr;
        return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                                  reinterpret_cast<LPCWSTR>(&keepModuleLoadedUntilProcessExit), &self) != 0;
    }();
    juce::ignoreUnused(pinned);
}

}
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
#if JUCE_WINDOWS
    keepModuleLoadedUntilProcessExit();
#endif
    return new hum::HumusProcessor();
}
