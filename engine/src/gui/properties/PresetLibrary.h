// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "core/packs/PackManifest.h"
#include "core/params/ParamSchema.h"
#include "io/PatchDocument.h"

namespace hum {

namespace presetlib {

inline juce::File libraryDir() { return userLibraryRoot().getChildFile("Presets"); }
inline juce::File groupFile(const juce::File& dir, const std::string& className, const std::string& group) {
    return juce::File(juce::String::fromUTF8(presetGroupFile(dir.getFullPathName().toStdString(), className, group).c_str()));
}
inline std::vector<PresetDef> loadTree(const juce::File& dir, const std::string& className) {
    return loadPresetTree(dir.getFullPathName().toStdString(), className);
}
inline juce::File libraryFile(const std::string& className, const std::string& group = {}) {
    return groupFile(libraryDir(), className, group);
}

inline std::vector<PresetDef> parse(const std::string& jsonText) { return parsePresetDefs(json::parse(jsonText)); }

inline void writeDef(juce::DynamicObject& o, const PresetDef& pd) {
    o.setProperty("name", juce::String(pd.name));
    auto* vals = new juce::DynamicObject();
    for (const auto& [n, v] : pd.values) vals->setProperty(juce::String(n), v);
    o.setProperty("values", juce::var(vals));
    if (!pd.texts.empty()) {
        auto* txts = new juce::DynamicObject();
        for (const auto& [n, t] : pd.texts)
            txts->setProperty(juce::String(n), juce::String(t));
        o.setProperty("texts", juce::var(txts));
    }
}

inline juce::var toVar(const std::vector<PresetDef>& defs) {
    juce::Array<juce::var> arr;
    for (const auto& pd : defs) {
        auto* o = new juce::DynamicObject();
        writeDef(*o, pd);
        arr.add(juce::var(o));
    }
    return juce::var(arr);
}

inline void writeGroup(const juce::File& dir, const std::string& className,
                       const std::string& group, const std::vector<PresetDef>& defs) {
    const auto f = groupFile(dir, className, group);
    if (defs.empty()) { f.deleteFile(); return; }
    f.getParentDirectory().createDirectory();
    f.replaceWithText(juce::JSON::toString(toVar(defs)));
}

inline void removeIn(const juce::File& dir, const std::string& className, const std::string& name) {
    const auto all = loadTree(dir, className);
    std::vector<std::string> touched;
    for (const auto& pd : all)
        if (pd.name == name && std::find(touched.begin(), touched.end(), pd.group) == touched.end())
            touched.push_back(pd.group);
    for (const auto& g : touched) {
        std::vector<PresetDef> keep;
        for (const auto& pd : all) if (pd.group == g && pd.name != name) keep.push_back(pd);
        writeGroup(dir, className, g, keep);
    }
}
inline void saveIn(const juce::File& dir, const std::string& className, const PresetDef& def) {
    removeIn(dir, className, def.name);
    std::vector<PresetDef> group;
    for (auto& pd : loadTree(dir, className)) if (pd.group == def.group) group.push_back(std::move(pd));
    group.push_back(def);
    writeGroup(dir, className, def.group, group);
}

inline std::vector<PresetDef> list(const std::string& className) { return loadTree(libraryDir(), className); }
inline void save(const std::string& className, const PresetDef& def) { saveIn(libraryDir(), className, def); }
inline void remove(const std::string& className, const std::string& name) { removeIn(libraryDir(), className, name); }

inline juce::var presetFileVar(const std::string& className, const PresetDef& def) {
    auto* o = new juce::DynamicObject();
    o->setProperty("humus", "preset");
    o->setProperty("class", juce::String(className));
    writeDef(*o, def);
    return juce::var(o);
}

inline std::string parsePresetFile(const std::string& jsonText, PresetDef& out) {
    const auto v = json::parse(jsonText);
    if (v["humus"].text() != "preset") return {};
    const auto cls = v["class"].text();
    if (cls.empty()) return {};
    auto defs = parsePresetDefs(json::Value::fromItems({v}));
    if (defs.size() != 1) return {};
    out = std::move(defs[0]);
    return cls;
}

inline PresetDef capture(const OrganismModel& c, const std::string& name) {
    PresetDef pd;
    pd.name = name;
    for (const auto& p : settingsOnly(c.properties)) {
        if (p.type == "text" || p.type == "soundfile" || p.type == "rhythmic-unit") {
            if (!p.text.empty()) pd.texts.emplace_back(p.name, p.text);
        } else {
            pd.values.emplace_back(p.name, p.value);
        }
    }
    return pd;
}

inline PresetModel toModel(const PresetDef& def, const std::vector<ParamDesc>& schema,
                           int number) {
    PresetModel pm;
    pm.number = number;
    pm.name = def.name;
    for (const auto& [pn, pv] : def.values) {
        Parameter p;
        p.name = pn;
        p.value = pv;
        p.type = "double";
        for (size_t si = 0; si < schema.size(); ++si)
            if (schema[si].name == pn) {
                p.index = (int) si;
                p.type = schema[si].isBool ? "bool"
                       : schema[si].isEnum ? "enum"
                       : schema[si].isInt  ? "int" : "double";
                break;
            }
        pm.properties.push_back(std::move(p));
    }
    for (const auto& [pn, pt] : def.texts) {
        Parameter p;
        p.name = pn;
        p.text = pt;
        p.type = "text";
        for (size_t si = 0; si < schema.size(); ++si)
            if (schema[si].name == pn) {
                p.index = (int) si;
                p.type = schema[si].isPlainText ? "text"
                       : schema[si].isText      ? "soundfile" : "rhythmic-unit";
                break;
            }
        pm.properties.push_back(std::move(p));
    }
    return pm;
}

}
}
