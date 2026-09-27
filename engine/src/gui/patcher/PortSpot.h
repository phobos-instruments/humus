// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_graphics/juce_graphics.h>

namespace hum {

enum class PortKind { Audio, Midi, Video, Control };

struct PortSpot {
    PortKind kind = PortKind::Audio;
    bool outlet = false;
    int index = 0;
    std::string name;
    juce::Point<int> at;
};

inline const char* portKindName(PortKind k) {
    switch (k) {
        case PortKind::Midi: return "midi";
        case PortKind::Video: return "video";
        case PortKind::Control: return "control";
        default: return "audio";
    }
}

}
