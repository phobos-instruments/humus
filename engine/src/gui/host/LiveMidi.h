// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "hum/caps/Midi.h"

namespace hum {

inline MidiEvent noteOnEvent(int channel, int note, int velocity) {
    MidiEvent e;
    e.data[0] = (unsigned char) (0x90 | ((channel - 1) & 0x0f));
    e.data[1] = (unsigned char) (note & 0x7f);
    e.data[2] = (unsigned char) (velocity & 0x7f);
    e.size = 3;
    return e;
}

inline MidiEvent noteOffEvent(int channel, int note) {
    MidiEvent e;
    e.data[0] = (unsigned char) (0x80 | ((channel - 1) & 0x0f));
    e.data[1] = (unsigned char) (note & 0x7f);
    e.size = 3;
    return e;
}

}
