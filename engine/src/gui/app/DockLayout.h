// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_graphics/juce_graphics.h>

namespace hum::docklayout {

struct Shown {
    bool center = true, right = true, bottom = false, left = false;
};

struct Span {
    int wanted = 0;
    int least = 0;
    int leaveOthers = 0;
};

struct Areas {
    juce::Rectangle<int> center, right, bottom, left, splitV, splitH, splitL;
};

inline int clampSpan(const Span& s, int room) {
    return juce::jlimit(s.least, juce::jmax(s.least, room - s.leaveOthers), s.wanted);
}

inline Areas lay(juce::Rectangle<int> area, Shown shown, Span rightW, Span bottomH, int splitThick, Span leftW = {}) {
    Areas out;
    const bool above = shown.center || shown.right || shown.left;
    if (shown.bottom) {
        if (!above) {
            out.bottom = area;
            return out;
        }
        out.bottom = area.removeFromBottom(clampSpan(bottomH, area.getHeight()));
        out.splitH = area.removeFromBottom(splitThick);
    }
    if (shown.left) {
        if (!shown.center && !shown.right) {
            out.left = area;
            return out;
        }
        out.left = area.removeFromLeft(clampSpan(leftW, area.getWidth()));
        out.splitL = area.removeFromLeft(splitThick);
    }
    if (shown.right && shown.center) {
        out.right = area.removeFromRight(clampSpan(rightW, area.getWidth()));
        out.splitV = area.removeFromRight(splitThick);
        out.center = area;
    } else if (shown.right) {
        out.right = area;
    } else if (shown.center) {
        out.center = area;
    }
    return out;
}

}
