// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <algorithm>
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/tracks/TracksLayout.h"
#include "hum/Pattern.h"

namespace hum::quantise {

constexpr int kMenuBase = 300;

struct Choice { const char* name; int ticks; };
constexpr Choice kChoices[] = {
    {"1/4",   Pattern::kTicksPerBeat},
    {"1/8",   Pattern::kTicksPerBeat / 2},
    {"1/8T",  Pattern::kTicksPerBeat / 3},
    {"1/16",  Pattern::kTicksPerBeat / 4},
    {"1/16T", Pattern::kTicksPerBeat / 6},
    {"1/32",  Pattern::kTicksPerBeat / 8},
};
constexpr int kCount = (int) (sizeof(kChoices) / sizeof(kChoices[0]));

inline juce::PopupMenu menu(double gridBeats) {
    juce::PopupMenu m;
    const int snap = std::max(1, (int) std::llround(gridBeats * Pattern::kTicksPerBeat));
    m.addItem(kMenuBase + kCount, "Snap grid (" + juce::String(trackslayout::gridLabel(gridBeats)) + ")");
    m.addSeparator();
    for (int i = 0; i < kCount; ++i)
        m.addItem(kMenuBase + i, kChoices[i].name, true, kChoices[i].ticks == snap);
    return m;
}

inline int ticksFor(int id, double gridBeats) {
    if (id < kMenuBase || id > kMenuBase + kCount) return -1;
    if (id == kMenuBase + kCount) return std::max(1, (int) std::llround(gridBeats * Pattern::kTicksPerBeat));
    return kChoices[id - kMenuBase].ticks;
}

}
