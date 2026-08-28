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
