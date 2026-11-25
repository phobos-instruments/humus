// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/NewOrganism.h"

#include <map>

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"

namespace hum {

std::string freeOrganismName(const PatchDocumentModel& doc, const std::string& plain) {
    std::string name = plain;
    for (int n = 2; doc.byName(name) != nullptr; ++n)
        name = plain + "_" + std::to_string(n);
    return name;
}

OrganismModel newOrganismModel(const std::string& className, const std::string& name,
                               const DefaultValue& defaultValue) {
    const std::string canonical = canonicalClass(className);
    OrganismModel cm;
    cm.name = name;
    cm.classRaw = canonical;
    const auto classId = parseClassString(canonical);
    cm.displayClass = classId.display;
    cm.kind = classId.kind;
    int idx = 0;
    for (const auto& d : schemaFor(canonical)) {
        Parameter p;
        p.index = idx++;
        p.name = d.name;
        if (d.isText) {
            p.type = d.isPlainText ? "text" : "soundfile";
            p.text = d.text;
        } else if (d.isRange) {
            p.type = "range";
            p.isRange = true;
            p.value = d.def;
            p.rangeMin = d.def;
            p.rangeMax = d.defMax;
        } else if (!d.text.empty()) {
            p.type = "rhythmic-unit";
            p.value = d.def;
            p.text = d.text;
        } else {
            p.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
            p.value = defaultValue ? defaultValue(d) : d.def;
        }
        cm.properties.push_back(p);
    }
    return cm;
}

OrganismModel reclassedOrganismModel(const OrganismModel& old, const std::string& newClass,
                                     const DefaultValue& defaultValue) {
    std::map<std::string, Parameter> kept;
    for (const auto& p : old.properties) kept[p.name] = p;

    OrganismModel cm;
    cm.name = old.name;
    cm.classRaw = newClass;
    const auto classId = parseClassString(newClass);
    cm.displayClass = classId.display;
    cm.kind = classId.kind;
    int idx = 0;
    const auto keptAs = [&kept](const ParamDesc& d) {
        auto it = kept.find(d.name);
        if (it != kept.end() || !d.isText) return it;
        const bool numbered = d.name.size() > 2 && d.name.compare(d.name.size() - 2, 2, "_1") == 0;
        return numbered ? kept.find(d.name.substr(0, d.name.size() - 2)) : kept.find(d.name + "_1");
    };
    for (const auto& d : schemaFor(newClass)) {
        const auto it = keptAs(d);
        Parameter p = it != kept.end() ? it->second : Parameter{};
        p.index = idx++;
        p.name = d.name;
        if (d.isText) p.type = d.isPlainText ? "text" : "soundfile";
        else if (it == kept.end() || !it->second.isRange)
            p.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
        if (it == kept.end()) {
            if (d.isText) p.text = d.text;
            else p.value = defaultValue ? defaultValue(d) : d.def;
        }
        cm.properties.push_back(p);
    }
    return cm;
}

}
