// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/packs/PresetDefs.h"
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
    int fixedTailOutlets = 0;
};

struct OrganismManifest {
    std::string dir;
    std::vector<OrganismClassManifest> classes;
};

bool parsePackManifest(const std::string& jsonText, PackManifest& out);
bool parseOrganismManifest(const std::string& jsonText, OrganismManifest& out);

const std::vector<PresetDef>& factoryPresetsFor(const std::string& className);

}
