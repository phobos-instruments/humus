// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <set>
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum {

inline std::vector<std::string> videoRenderOrder(
    const std::string& root, const std::vector<ConnectionModel>& cords) {
    std::vector<std::string> order;
    std::set<std::string> done, onPath;
    struct Frame { std::string node; bool expanded; };
    std::vector<Frame> stack{{root, false}};
    while (!stack.empty()) {
        auto [node, expanded] = stack.back();
        stack.pop_back();
        if (expanded) {
            onPath.erase(node);
            if (done.insert(node).second) order.push_back(node);
            continue;
        }
        if (done.count(node) || onPath.count(node)) continue;
        onPath.insert(node);
        stack.push_back({node, true});
        for (const auto& c : cords)
            if (c.dst == node && !done.count(c.src) && !onPath.count(c.src))
                stack.push_back({c.src, false});
    }
    return order;
}

}
