// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/StateChanges.h"

#include "core/xml/Xml.h"
#include "io/PatchWriteInternal.h"

namespace hum {

namespace {

std::string loadedStateOf(const OrganismModel& c) {
    xml::Element root("state");
    for (const auto& p : c.properties) writeProperty(root, p);
    if (c.pattern.present) writePattern(root, c.pattern);
    return xml::write(root);
}

bool sameClass(const OrganismModel& a, const OrganismModel& b) {
    return a.classRaw == b.classRaw && a.displayClass == b.displayClass && a.kind == b.kind;
}

}

std::vector<std::string> organismsWhoseStateChanged(const PatchDocumentModel& before, const PatchDocumentModel& after) {
    std::vector<std::string> out;
    for (const auto& now : after.organisms) {
        const auto* was = before.byName(now.name);
        if (was == nullptr || !sameClass(*was, now)) continue;
        if (loadedStateOf(*was) != loadedStateOf(now)) out.push_back(now.name);
    }
    return out;
}

}
