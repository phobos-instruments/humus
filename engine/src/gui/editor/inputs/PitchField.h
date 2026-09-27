// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include "core/midi/MidiFormat.h"
#include "hum/dsp/DspMath.h"

namespace hum::pitchfield {

inline constexpr int kCentsShownFrom = 5;

inline int nearestNote(double hz) {
    return std::clamp((int) std::lround(hzToMidi(hz)), 0, (int) kMidiMax);
}

inline int centsOff(double hz) {
    return (int) std::lround((hzToMidi(hz) - (double) nearestNote(hz)) * 100.0);
}

inline std::string label(double hz) {
    const int cents = centsOff(hz);
    std::string text = midiNoteName(nearestNote(hz));
    if (std::abs(cents) >= kCentsShownFrom) text += (cents > 0 ? " +" : " ") + std::to_string(cents);
    return text;
}

inline int lowestNote(double minHz) { return std::clamp((int) std::ceil(hzToMidi(minHz) - 1.0e-9), 0, (int) kMidiMax); }
inline int highestNote(double maxHz) { return std::clamp((int) std::floor(hzToMidi(maxHz) + 1.0e-9), 0, (int) kMidiMax); }

inline double stepped(double hz, int direction, double minHz, double maxHz) {
    const int cents = centsOff(hz);
    int note = nearestNote(hz);
    if (direction > 0 && cents >= 0) ++note;
    if (direction < 0 && cents <= 0) --note;
    note = std::clamp(note, lowestNote(minHz), highestNote(maxHz));
    return midiToHz((double) note);
}

}
