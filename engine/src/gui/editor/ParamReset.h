// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/ParamRanges.h"
#include "gui/host/BrickHost.h"

namespace hum {

inline void enableDoubleClickReset(juce::Slider& s, BrickHost& host,
                                   const std::string& organism, const std::string& param) {
    double def = 0.0, defMax = 0.0;
    if (paramDefault(host, organism, param, def, defMax))
        s.setDoubleClickReturnValue(true, def);
}

}
