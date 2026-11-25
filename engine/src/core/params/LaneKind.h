// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/params/ParamSchema.h"

namespace hum {

inline const ParamDesc* paramDescOf(const std::vector<ParamDesc>& schema, const std::string& param) {
    for (const auto& d : schema)
        if (d.name == param) return &d;
    return nullptr;
}

inline std::string laneKindFor(const ParamDesc* desc, bool isRange, bool heldClockParam) {
    if (desc != nullptr && desc->isTrigger) return "trigger";
    if (isRange) return "range";
    if (heldClockParam || (desc != nullptr && desc->isEnum)) return "step";
    return "double";
}

}
