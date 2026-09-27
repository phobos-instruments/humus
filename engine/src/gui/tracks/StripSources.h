// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum {

inline std::vector<int> stripChannelInlets(const std::string& param) {
    const auto us = param.rfind('_');
    if (us == std::string::npos || us + 1 >= param.size()) return {};
    const auto sfx = param.substr(us + 1);
    int lo = 0, hi = 0;
    const auto dash = sfx.find('-');
    for (char ch : sfx)
        if ((ch < '0' || ch > '9') && ch != '-') return {};
    if (dash == std::string::npos) {
        lo = hi = std::atoi(sfx.c_str());
    } else {
        lo = std::atoi(sfx.substr(0, dash).c_str());
        hi = std::atoi(sfx.substr(dash + 1).c_str());
    }
    if (lo < 1 || hi < lo || hi - lo > 15) return {};
    std::vector<int> out;
    for (int c = lo; c <= hi; ++c) out.push_back(c - 1);
    return out;
}

inline std::vector<std::string> stripSourceNames(const std::string& param,
                                                 const std::vector<ConnectionModel>& cords,
                                                 const std::string& node) {
    const auto inlets = stripChannelInlets(param);
    std::vector<std::string> names;
    for (const auto& c : cords) {
        if (c.dst != node) continue;
        if (std::find(inlets.begin(), inlets.end(), c.dstInlet) == inlets.end()) continue;
        if (std::find(names.begin(), names.end(), c.src) == names.end()) names.push_back(c.src);
    }
    return names;
}

inline std::string joinedSources(const std::vector<std::string>& names) {
    std::string out;
    for (const auto& n : names) out += (out.empty() ? "" : " + ") + n;
    return out;
}

}
