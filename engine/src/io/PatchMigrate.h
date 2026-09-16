// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/packs/ClassString.h"
#include "core/params/ParamSchema.h"
#include "io/ModRouteBuild.h"
#include "io/PatchDocument.h"

namespace hum {

inline const char* legacyControlOutletValue(const std::string& cls, int outlet) {
    if (cls == "Number" || cls == "Slider" || cls == "Button") return outlet == 0 ? "value" : nullptr;
    if (cls == "Follower") return outlet == 0 ? "env" : outlet == 1 ? "gate" : nullptr;
    if (cls == "LFO") return outlet == 0 ? "wave" : nullptr;
    if (cls == "SerialIn") return outlet == 0 ? "a" : outlet == 1 ? "b" : nullptr;
    return nullptr;
}

inline const char* legacySocketParam(const std::string& cls, int inlet) {
    if (cls == "VCA") return inlet == 1 ? "Gain" : nullptr;
    if (cls == "Number" || cls == "Slider") return inlet == 0 ? "Value" : nullptr;
    if (cls == "Button") return inlet == 0 ? "Press" : nullptr;
    if (cls == "LFO") return inlet == 0 ? "Offset" : nullptr;
    if (cls == "SerialOut") return inlet == 0 ? "Value1" : inlet == 1 ? "Value2" : nullptr;
    return nullptr;
}

inline void migrateLegacyControl(PatchDocumentModel& doc) {
    auto classOf = [&](const std::string& name) -> std::string {
        const auto* cm = doc.byName(name);
        return cm == nullptr ? std::string() : cm->displayClass;
    };
    std::vector<ConnectionModel> kept;
    for (const auto& c : doc.connections) {
        const auto srcCls = classOf(c.src);
        const char* value = legacyControlOutletValue(srcCls, c.srcOutlet);
        if (value == nullptr) { kept.push_back(c); continue; }
        const auto dstCls = classOf(c.dst);
        const char* param = legacySocketParam(dstCls, c.dstInlet);
        if (param == nullptr) {
            doc.migrationNotes.push_back(c.src + " -> " + c.dst + " inlet " + std::to_string(c.dstInlet + 1)
                                         + " dropped: a control signal into an audio inlet");
            continue;
        }
        auto* target = const_cast<OrganismModel*>(doc.byName(c.dst));
        ModControllerSource s;
        s.propertyName = param;
        s.propertyIndex = -1;
        s.sourceOrganism = c.src;
        s.sourceValue = value;
        s.mapMin = 0.0;
        s.mapMax = 1.0;
        target->modSources.push_back(std::move(s));
        doc.migrationNotes.push_back(c.src + " -> " + c.dst + " is now a control cord onto " + param);
    }
    doc.connections.swap(kept);
    for (auto& cm : doc.organisms) {
        if (cm.displayClass != "VCA") continue;
        cm.classRaw = "Gain";
        cm.displayClass = "Gain";
        cm.kind = parseClassString(cm.classRaw).kind;
        std::vector<Parameter> props;
        for (const auto& p : cm.properties) if (p.name != "Bipolar") props.push_back(p);
        cm.properties.swap(props);
        doc.migrationNotes.push_back(cm.name + " was a VCA and is now a Gain");
    }
}

}
