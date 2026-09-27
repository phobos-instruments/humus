// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <initializer_list>

#include <juce_graphics/juce_graphics.h>

namespace hum {

inline juce::String firstInstalledOf(std::initializer_list<const char*> wanted) {
    const auto installed = juce::Font::findAllTypefaceNames();
    for (const auto* name : wanted)
        if (installed.contains(name)) return name;
    return {};
}

inline juce::String platformUiTypeface() {
#if JUCE_WINDOWS
    return firstInstalledOf({"Segoe UI"});
#elif JUCE_LINUX || JUCE_BSD
    return firstInstalledOf({"DejaVu Sans", "Liberation Sans", "Noto Sans"});
#else
    return {};
#endif
}

}
