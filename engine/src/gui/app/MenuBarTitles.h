// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

struct MenuBarTitle {
    std::string item, menu;
};

#if JUCE_MAC
void hostMenuBarWithoutDockIcon();
std::vector<MenuBarTitle> menuBarTitles();
#else
inline void hostMenuBarWithoutDockIcon() {}
inline std::vector<MenuBarTitle> menuBarTitles() { return {}; }
#endif

}
