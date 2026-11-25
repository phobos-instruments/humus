// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/params/ParamSchema.h"
#include "core/timeline/Automation.h"
#include "io/PatchDocument.h"

namespace hum {

inline AutoKind autoKindOf(const std::string& kind) {
    if (kind == "range") return AutoKind::Range;
    if (kind == "trigger") return AutoKind::Trigger;
    if (kind == "step") return AutoKind::Step;
    return AutoKind::Double;
}

inline AutoLane autoLaneOf(const OrganismModel& cm, const AutomationLane& ml, int node) {
    AutoLane lane;
    lane.node = node;
    lane.param = ml.propertyName;
    lane.mute = ml.mute;
    lane.kind = autoKindOf(ml.kind);
    for (const auto& d : schemaFor(cm.classRaw))
        if (d.name == ml.propertyName) {
            lane.rest = d.min;
            lane.pulse = d.max;
            lane.base = d.def;
            break;
        }
    for (const auto& p : cm.properties)
        if (p.name == ml.propertyName) {
            lane.base = p.value;
            break;
        }
    for (const auto& bp : ml.points) lane.points.push_back({bp.beat, bp.value, bp.valueMax, bp.curve, bp.release});
    return lane;
}

}
