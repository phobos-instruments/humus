// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "core/params/ParamSchema.h"
#include "hum/LayoutSpec.h"

namespace hum::brick {

inline bool inFamily(const std::string& param, const std::string& prefix) {
    return param.rfind(prefix, 0) == 0;
}

inline std::vector<std::string> familyOf(const std::vector<ParamDesc>& schema,
                                         const std::string& prefix) {
    std::vector<std::string> out;
    for (const auto& d : schema)
        if (inFamily(d.name, prefix)) out.push_back(d.name);
    return out;
}

inline int familySize(const std::vector<ParamDesc>& schema, const std::string& prefix) {
    return (int) familyOf(schema, prefix).size();
}

struct Fader {
    std::string param;
    std::string labelSuffix;
    double min = 0.0, max = 1.0;
};

inline std::vector<Fader> faderFamily(const std::vector<ParamDesc>& schema,
                                      const std::string& prefix) {
    std::vector<Fader> out;
    for (const auto& d : schema)
        if (inFamily(d.name, prefix))
            out.push_back({d.name, d.name.substr(prefix.size()), d.min, d.max});
    return out;
}

inline std::vector<std::string> csvFields(const std::string& spec) {
    std::vector<std::string> out;
    for (size_t i = 0, start = 0; i <= spec.size(); ++i)
        if (i == spec.size() || spec[i] == ',') {
            auto field = spec.substr(start, i - start);
            while (!field.empty() && field.front() == ' ') field.erase(field.begin());
            while (!field.empty() && field.back() == ' ') field.pop_back();
            if (!field.empty()) out.push_back(field);
            start = i + 1;
        }
    return out;
}

inline int atLeastOne(const std::string& extra) {
    return std::max(1, std::atoi(extra.c_str()));
}

struct StepGridSetup {
    bool arp = false;
    int steps = 16;
    std::string matrixId;
    std::string seed;
};

inline StepGridSetup stepGridSetupFor(const LayoutSpec::Control& s) {
    StepGridSetup grid;
    grid.arp = s.extraOr("mode", "bassline") == "arp";
    grid.steps = std::atoi(s.extraOr("steps", grid.arp ? "32" : "16").c_str());
    grid.matrixId = grid.arp ? "trigger-tie-matrix" : "bassline-pattern-matrix";
    grid.seed = s.extraOr("seed", "");
    return grid;
}

}
