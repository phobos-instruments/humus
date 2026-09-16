// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>

#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/dsp/DspMath.h"

namespace hum::timelinechrome {

struct NotePlot {
    float perTick = 0.0f;
    float top = 0.0f;
    float h = 0.0f;
    float rowH = 0.0f;
    int lo = 0, span = 1;
    bool single = false;
    bool dense = false;
    bool usable = false;

    float yFor(int pitch) const {
        if (single || span <= 0 || rowH >= h) return top + (h - rowH) * 0.5f;
        const float t = (float) (pitch - lo) / (float) span;
        return top + h - rowH - t * (h - rowH);
    }
};

inline NotePlot notePlot(juce::Rectangle<int> brick, int fullW, int lengthTicks,
                         int lo, int hi, int ticksPerBeat) {
    NotePlot np;
    if (fullW <= 0 || lengthTicks <= 0 || ticksPerBeat <= 0) return np;
    np.perTick = (float) fullW / (float) lengthTicks;
    np.top = (float) brick.getY() + 3.0f;
    np.h = (float) brick.getHeight() - 6.0f;
    if (np.h <= 2.0f) return np;
    np.lo = juce::jmin(lo, hi);
    np.single = hi == lo;
    np.span = juce::jmax(1, hi - lo);
    np.rowH = juce::jmax(1.4f, np.h / (float) (np.span + 1));
    np.dense = np.perTick * (float) ticksPerBeat < 8.0f;
    np.usable = true;
    return np;
}

inline bool isBlackKey(int pitch) {
    static const bool black[12] = {false, true, false, true, false, false,
                                   true, false, true, false, true, false};
    return black[(std::size_t) (((pitch % 12) + 12) % 12)];
}

struct RollPlot {
    float top = 0.0f, h = 0.0f;
    float rowH = 10.0f;
    int topPitch = 83;
    bool usable = false;

    float hFor(int) const { return rowH; }
    float yFor(int pitch) const { return top + (float) (topPitch - pitch) * rowH; }
    int pitchAt(float y) const {
        return topPitch - (int) std::floor((y - top) / juce::jmax(0.001f, rowH));
    }
    int rows() const { return (int) std::ceil(h / juce::jmax(0.001f, rowH)) + 1; }
};

inline RollPlot rollPlot(float top, float h, int loPitch, int hiPitch,
                         int scrollSemis = 0, float wantedRowH = 10.0f) {
    RollPlot rp;
    rp.top = top;
    rp.h = h;
    if (h < 12.0f) return rp;
    int lo = juce::jlimit(0, kMidiMax, juce::jmin(loPitch, hiPitch) - 2);
    int hi = juce::jlimit(0, kMidiMax, juce::jmax(loPitch, hiPitch) + 2);
    if (hi - lo < 11) hi = juce::jmin(kMidiMax, lo + 11);
    const int span = juce::jmax(1, hi - lo + 1);
    rp.rowH = juce::jlimit(3.0f, 24.0f, juce::jmin(wantedRowH, h / (float) span));
    const int visible = (int) std::floor(h / rp.rowH);
    rp.topPitch = juce::jlimit(0, kMidiMax, hi + std::max(0, (visible - span) / 2) + scrollSemis);
    rp.usable = true;
    return rp;
}

}
