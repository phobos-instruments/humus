// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "core/json/Json.h"

namespace hum {

struct PresetDef {
    std::string name;
    std::string group;
    std::vector<std::pair<std::string, double>> values;
    std::vector<std::pair<std::string, std::string>> texts;
};

std::vector<PresetDef> parsePresetDefs(const json::Value& v, const std::string& group = {});
std::string presetGroupOf(const std::string& path);
std::string presetGroupFile(const std::string& dir, const std::string& className, const std::string& group);
std::vector<PresetDef> loadPresetTree(const std::string& dir, const std::string& className);

}
