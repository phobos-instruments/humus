// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin/DesktopSession.h"

#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace hum {

bool sessionCanOpenWindows() {
#if JUCE_WINDOWS
    USEROBJECTFLAGS flags{};
    const auto station = GetProcessWindowStation();
    return station != nullptr && GetUserObjectInformationW(station, UOI_FLAGS, &flags, sizeof(flags), nullptr)
           && (flags.dwFlags & WSF_VISIBLE) != 0;
#else
    return juce::Desktop::getInstance().getDisplays().getPrimaryDisplay() != nullptr;
#endif
}

}
