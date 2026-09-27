// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {

inline int clampMidi7(int v) { return v < 0 ? 0 : (v > kMidiMax ? kMidiMax : v); }

struct MidiOutMessage {
    int status = 0;
    int data1 = 0;
    int data2 = 0;
};

inline std::vector<MidiOutMessage> midiOutMessages(const std::string& changedParam,
                                                   int channel, int controller, int value,
                                                   int note, int velocity, double gate) {
    const int ch = (clampMidi7(channel < 1 ? 1 : channel) - 1) & 0x0F;
    std::vector<MidiOutMessage> out;
    if (changedParam == "Value" || changedParam == "Controller")
        out.push_back({0xB0 | ch, clampMidi7(controller), clampMidi7(value)});
    else if (changedParam == "Gate")
        out.push_back(gate >= 0.5
                          ? MidiOutMessage{0x90 | ch, clampMidi7(note), clampMidi7(velocity)}
                          : MidiOutMessage{0x80 | ch, clampMidi7(note), 0});
    return out;
}

}
