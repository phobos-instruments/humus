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

inline bool meterWithoutOutlets(const std::string& cls) {
    return cls == "VuMeter" || cls == "Scope" || cls == "Spectrum";
}

inline void bypassMeterOutlets(PatchDocumentModel& doc, const OrganismModel& meter) {
    std::vector<ConnectionModel> feeders[2], kept, through;
    for (const auto& c : doc.connections) {
        if (c.dst == meter.name && c.dstInlet >= 0 && c.dstInlet < 2) feeders[c.dstInlet].push_back(c);
        if (c.src == meter.name) through.push_back(c);
        else kept.push_back(c);
    }
    if (through.empty()) return;
    auto already = [&](const ConnectionModel& c) {
        for (const auto& k : kept)
            if (k.src == c.src && k.srcOutlet == c.srcOutlet && k.dst == c.dst && k.dstInlet == c.dstInlet) return true;
        return false;
    };
    for (const auto& out : through) {
        const int channel = out.srcOutlet == 1 && !feeders[1].empty() ? 1 : 0;
        if (out.srcOutlet > 1) continue;
        for (const auto& in : feeders[channel]) {
            ConnectionModel direct = out;
            direct.src = in.src;
            direct.srcOutlet = in.srcOutlet;
            if (!already(direct)) kept.push_back(direct);
        }
    }
    doc.connections.swap(kept);
    doc.migrationNotes.push_back(meter.name + ": meters no longer pass sound through, so its input now goes straight to the next box");
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
            doc.migrationNotes.push_back("the cord from " + c.src + " to " + c.dst + " (inlet " + std::to_string(c.dstInlet + 1)
                                         + ") was removed: it carried a control value into an audio inlet");
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
        doc.migrationNotes.push_back("the cord from " + c.src + " to " + c.dst + " now controls its " + param + " knob");
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
        doc.migrationNotes.push_back(cm.name + ": the VCA is now a Gain, which does the same job");
    }
    for (const auto& cm : doc.organisms)
        if (meterWithoutOutlets(cm.displayClass)) bypassMeterOutlets(doc, cm);
    for (auto& cm : doc.organisms) {
        if (cm.classRaw != "Sequence") continue;
        cm.classRaw = "Sequence8";
        cm.displayClass = "Sequence8";
    }
    for (auto& cm : doc.organisms) {
        if (cm.displayClass != "Rings") continue;
        cm.classRaw = "Atom";
        cm.displayClass = "Atom";
        cm.kind = parseClassString(cm.classRaw).kind;
        doc.migrationNotes.push_back(cm.name + ": Rings is now called Atom");
    }
    for (auto& cm : doc.organisms) {
        if (cm.displayClass != "SoundSpace") continue;
        bool hasSkew = false;
        for (const auto& p : cm.properties) hasSkew = hasSkew || p.name == "Skew";
        if (hasSkew) continue;
        for (auto& p : cm.properties) {
            if (p.name != "Shape") continue;
            p.name = "Skew";
            doc.migrationNotes.push_back(cm.name + ": the Shape knob is now called Skew. It sounds the same");
        }
    }
    for (auto& cm : doc.organisms) {
        if (cm.displayClass != "5Combs") continue;
        for (auto& p : cm.properties) {
            if (p.name != "InputGain" || p.value != 0.0) continue;
            p.value = 1.0;
            doc.migrationNotes.push_back(cm.name + ": its master level was 0 and is now 1, as the original organism played it");
        }
    }
}

}
