// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_graphics/juce_graphics.h>

#include "gui/style/Colours.h"
#include "hum/caps/Files.h"

namespace hum {

inline juce::Colour strandStateColour(int state) {
    switch (state) {
        case StrandStatus::kRecord: return ink::strand::mute;
        case StrandStatus::kDub: return ink::strand::solo;
        case StrandStatus::kPlay: return Palette::accent;
        case StrandStatus::kStopped: return Palette::textDim;
        default: return Palette::border;
    }
}

}
