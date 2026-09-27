// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "core/graph/PodModel.h"
#include "io/PatchDocument.h"

namespace hum::tape {

inline constexpr const char* kJoin = " + ";

inline std::string fromCords(const PatchDocumentModel& m, const std::string& node, int firstInlet, int width) {
    std::vector<std::string> names;
    for (const auto& c : m.connections) {
        if (c.dst != node || c.dstInlet < firstInlet || c.dstInlet >= firstInlet + width) continue;
        const auto leaf = pods::leafOf(c.src);
        if (std::find(names.begin(), names.end(), leaf) == names.end()) names.push_back(leaf);
    }
    std::string out;
    for (const auto& n : names) out += (out.empty() ? "" : kJoin) + n;
    return out;
}

inline std::string shown(const std::string& typed, const std::string& corded) {
    return typed.empty() ? corded : typed;
}

inline std::string toStore(const std::string& typed, const std::string& corded) {
    const auto first = typed.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    const auto trimmed = typed.substr(first, typed.find_last_not_of(' ') - first + 1);
    return trimmed == corded ? std::string() : trimmed;
}

}
