// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>
#include <utility>
#include <vector>

#include <juce_graphics/juce_graphics.h>

#include "io/PatchDocument.h"

namespace hum {

struct PodClip {
    std::string leaf;
    std::vector<OrganismModel> nodes;
    std::vector<ConnectionModel> cords, midiCords, videoCords;
    std::vector<std::pair<std::string, juce::Point<int>>> layout;
    juce::Point<int> boxPos;
};

}
