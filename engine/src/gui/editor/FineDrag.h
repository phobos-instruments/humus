// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/AppSettings.h"

namespace hum {

inline double fineDragFactor() {
    return (double) juce::jlimit(2, 50, AppSettings::instance().getInt("controls.fineDrag", 10));
}

inline void applyFineCrawl(juce::Slider& s) {
    s.setVelocityModeParameters(0.8 / fineDragFactor(), 1, 0.0, true,
                                juce::ModifierKeys::shiftModifier);
}

}
