// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

#include "gui/editor/ControlArt.h"
#include "hum/LayoutSpec.h"

namespace hum::art {

inline const std::vector<std::string>& imageKeys() {
    static const std::vector<std::string> keys{"art", "art-on", "art-down", "art-track", "art-arrow"};
    return keys;
}

inline const std::vector<std::string>& settingKeys() {
    static const std::vector<std::string> keys{"art-frames", "art-slice", "art-angles", "art-text", "art-caption"};
    return keys;
}

inline bool isArtKey(const std::string& key) { return key == "art" || key.rfind("art-", 0) == 0; }

inline bool wearsArt(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    switch (type) {
        case CT::Knob: case CT::VSlider: case CT::HSlider: case CT::RotarySwitch: case CT::Momentary:
        case CT::Toggle: case CT::MiniToggle: case CT::LitButton: case CT::EnumButtons: case CT::Combo: return true;
        default: return false;
    }
}

inline std::string doubleSizeOf(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return path + "@2x";
    return path.substr(0, dot) + "@2x" + path.substr(dot);
}

inline bool isCount(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

inline bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    std::strtod(s.c_str(), &end);
    return end == s.c_str() + s.size();
}

inline bool isAngles(const std::string& s) {
    const auto comma = s.find(',');
    return comma != std::string::npos && isNumber(s.substr(0, comma)) && isNumber(s.substr(comma + 1));
}

inline bool isHexColour(const std::string& s) {
    const auto hex = s.empty() || s.front() != '#' ? s : s.substr(1);
    return (hex.size() == 6 || hex.size() == 8)
           && std::all_of(hex.begin(), hex.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
}

inline std::string settingProblem(const std::string& key, const std::string& value) {
    if ((key == "art-frames" || key == "art-slice") && !isCount(value)) return "'" + key + "' is not a whole number";
    if (key == "art-angles" && !isAngles(value)) return "'art-angles' is not two degrees like \"-135,135\"";
    if (key == "art-text" && !isHexColour(value)) return "'art-text' is not a #RRGGBB colour";
    if (key == "art-caption" && value != "true" && value != "false") return "'art-caption' is not true or false";
    return {};
}

template <class Exists>
std::vector<std::string> problemsFor(const LayoutSpec& spec, Exists&& exists) {
    std::vector<std::string> out;
    auto known = [](const std::vector<std::string>& keys, const std::string& key) {
        return std::find(keys.begin(), keys.end(), key) != keys.end();
    };
    for (size_t i = 0; i < spec.controls.size(); ++i) {
        const auto& c = spec.controls[i];
        const auto who = (c.param.empty() ? std::string(controlTypeToName(c.type)) + " " + std::to_string(i) : c.param)
                         + ": ";
        const bool hasArt = !c.extraOr("art").empty();
        bool named = false;
        for (const auto& [key, value] : c.extra) {
            if (!isArtKey(key)) continue;
            named = true;
            if (!known(imageKeys(), key) && !known(settingKeys(), key)) {
                out.push_back(who + "unknown key '" + key + "'");
                continue;
            }
            if (!hasArt && key != "art") {
                out.push_back(who + "'" + key + "' without 'art'");
                continue;
            }
            if (known(settingKeys(), key)) {
                if (const auto why = settingProblem(key, value); !why.empty()) out.push_back(who + why);
                continue;
            }
            const auto path = pathIn(spec.dir, value);
            if (value.empty() || (!exists(path) && !exists(doubleSizeOf(path))))
                out.push_back(who + "no image at '" + value + "' for '" + key + "'");
        }
        if (named && !wearsArt(c.type))
            out.push_back(who + "a " + controlTypeToName(c.type) + " does not wear art");
    }
    return out;
}

}
