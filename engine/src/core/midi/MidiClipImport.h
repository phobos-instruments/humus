// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "hum/PatternMatrix.h"

namespace hum::midiclip {

struct Imported {
    std::vector<NoteEvent> notes;
    int lengthTicks = 0;
};

Imported importMidi(const std::uint8_t* data, std::size_t size);

}
