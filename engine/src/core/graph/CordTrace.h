// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "io/PatchDocument.h"

namespace hum::cords {

inline std::string destinationOf(const PatchDocumentModel& m, const std::string& node) {
    for (const auto* list : {&m.midiConnections, &m.connections, &m.videoConnections})
        for (const auto& c : *list)
            if (c.src == node) return c.dst;
    return {};
}

}
