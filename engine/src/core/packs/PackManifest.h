// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/params/ParamSchema.h"

namespace hum {

struct PackManifest {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string origin;
    std::string license;
};

struct PresetDef {
    std::string name;
    std::string group;
    std::vector<std::pair<std::string, double>> values;
    std::vector<std::pair<std::string, std::string>> texts;
};

inline std::vector<PresetDef> parsePresetDefs(const juce::var& v, const std::string& group = {}) {
    std::vector<PresetDef> out;
    if (const auto* arr = v.getArray())
        for (const auto& e : *arr) {
            PresetDef pd;
            pd.name = e["name"].toString().trim().toStdString();
            pd.group = group;
            if (pd.name.empty()) continue;
            if (const auto* vo = e["values"].getDynamicObject())
                for (const auto& kv : vo->getProperties()) {
                    if (kv.value.isString())
                        pd.texts.emplace_back(kv.name.toString().toStdString(),
                                              kv.value.toString().toStdString());
                    else
                        pd.values.emplace_back(kv.name.toString().toStdString(), (double) kv.value);
                }
            if (const auto* to = e["texts"].getDynamicObject())
                for (const auto& kv : to->getProperties())
                    pd.texts.emplace_back(kv.name.toString().toStdString(),
                                          kv.value.toString().toStdString());
            out.push_back(std::move(pd));
        }
    return out;
}

inline std::string presetGroupOf(const juce::File& f) {
    auto n = f.getFileNameWithoutExtension().trim();
    int i = 0;
    while (i < n.length() && juce::CharacterFunctions::isDigit(n[i])) ++i;
    return (i > 0 && i < n.length() && n[i] == ' ') ? n.substring(i + 1).trim().toStdString()
                                                    : n.toStdString();
}
inline juce::File presetGroupFile(const juce::File& dir, const std::string& className,
                                  const std::string& group) {
    if (group.empty()) return dir.getChildFile(juce::String(className) + ".json");
    const auto cdir = dir.getChildFile(juce::String(className));
    for (const auto& f : cdir.findChildFiles(juce::File::findFiles, false, "*.json"))
        if (presetGroupOf(f) == group) return f;
    return cdir.getChildFile(juce::String(group) + ".json");
}
inline std::vector<PresetDef> loadPresetTree(const juce::File& dir, const std::string& className) {
    std::vector<PresetDef> out;
    const auto flat = presetGroupFile(dir, className, {});
    if (flat.existsAsFile())
        for (auto& pd : parsePresetDefs(juce::JSON::parse(flat.loadFileAsString()))) out.push_back(std::move(pd));
    auto files = dir.getChildFile(juce::String(className)).findChildFiles(juce::File::findFiles, false, "*.json");
    files.sort();
    for (const auto& f : files)
        for (auto& pd : parsePresetDefs(juce::JSON::parse(f.loadFileAsString()), presetGroupOf(f)))
            out.push_back(std::move(pd));
    return out;
}

struct OrganismClassManifest {
    std::string className;
    std::string category;
    std::vector<ParamDesc> params;
    std::vector<PresetDef> presets;
    std::string editor;
    std::string help;
    std::string blurb;
    std::string caution;
    std::string cautionIcon;
    bool hidden = false;
    std::string canonical;
    bool strips = false;
    std::vector<std::string> roles;
    std::string roll;
};

struct OrganismManifest {
    std::string dir;
    std::vector<OrganismClassManifest> classes;
};

bool parsePackManifest(const std::string& jsonText, PackManifest& out);
bool parseOrganismManifest(const std::string& jsonText, OrganismManifest& out);

const std::vector<PresetDef>& factoryPresetsFor(const std::string& className);

}
