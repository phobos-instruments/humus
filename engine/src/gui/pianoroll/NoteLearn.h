// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <vector>

#include "core/midi/MidiSource.h"

namespace hum::notelearn {

inline int learnedNote(int lastSource, const std::vector<int>& heldBefore, const std::vector<int>& heldNow,
                       int lo, int hi) {
    const auto inRange = [lo, hi](int note) { return note >= lo && note <= hi; };
    if (isNoteSource(lastSource) && inRange(noteOfSource(lastSource))) return noteOfSource(lastSource);
    for (int source : heldNow)
        if (isNoteSource(source) && inRange(noteOfSource(source))
            && std::find(heldBefore.begin(), heldBefore.end(), source) == heldBefore.end())
            return noteOfSource(source);
    return -1;
}

}
