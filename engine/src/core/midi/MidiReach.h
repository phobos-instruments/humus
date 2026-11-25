// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "hum/caps/Midi.h"

namespace hum {

inline constexpr int kAllMidiChannels = 0;

struct MidiReach {
    int port = kLiveMidiAllPorts;
    int channel = kAllMidiChannels;

    bool allChannels() const { return channel < 1 || channel > 16; }
    bool hears(int fromPort, int fromChannel) const {
        return (port == kLiveMidiAllPorts || port == fromPort) && (allChannels() || channel == fromChannel);
    }
    bool operator<(const MidiReach& o) const { return port != o.port ? port < o.port : channel < o.channel; }
    bool operator==(const MidiReach& o) const { return port == o.port && channel == o.channel; }
};

}
