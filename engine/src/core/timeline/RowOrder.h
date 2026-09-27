// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace hum::roworder {

inline constexpr int kUnplaced = -1;

struct Row {
    std::string name;
    int index = kUnplaced;
    std::string pod;
    std::string above;
};

inline int positionOf(const std::vector<std::string>& order, const std::string& name) {
    const auto it = std::find(order.begin(), order.end(), name);
    return it == order.end() ? -1 : (int) (it - order.begin());
}

inline std::vector<std::string> podsKeptTogether(const std::vector<Row>& rows) {
    std::vector<std::string> out;
    std::set<std::string> gathered;
    for (const auto& r : rows) {
        if (r.pod.empty()) {
            out.push_back(r.name);
            continue;
        }
        if (!gathered.insert(r.pod).second) continue;
        for (const auto& member : rows)
            if (member.pod == r.pod) out.push_back(member.name);
    }
    return out;
}

inline std::vector<std::string> arranged(const std::vector<Row>& natural) {
    std::vector<Row> placed, anchored, loose;
    for (const auto& r : natural) {
        if (r.index != kUnplaced) placed.push_back(r);
        else if (!r.above.empty()) anchored.push_back(r);
        else loose.push_back(r);
    }
    std::stable_sort(placed.begin(), placed.end(),
                     [](const Row& a, const Row& b) { return a.index < b.index; });
    for (auto& r : loose) placed.push_back(std::move(r));
    for (auto& r : anchored) {
        auto at = std::find_if(placed.begin(), placed.end(),
                               [&](const Row& p) { return p.name == r.above; });
        placed.insert(at, std::move(r));
    }
    return podsKeptTogether(placed);
}

inline std::vector<std::string> moved(const std::vector<std::string>& order,
                                      const std::set<std::string>& moving, int before) {
    std::vector<std::string> block, rest;
    int landing = 0;
    for (int i = 0; i < (int) order.size(); ++i) {
        const bool rides = moving.count(order[(size_t) i]) != 0;
        (rides ? block : rest).push_back(order[(size_t) i]);
        if (!rides && i < before) ++landing;
    }
    rest.insert(rest.begin() + landing, block.begin(), block.end());
    return rest;
}

inline std::vector<std::string> placedBelow(const std::vector<std::string>& order,
                                            const std::string& name, const std::string& anchor) {
    const int at = positionOf(order, anchor);
    if (positionOf(order, name) < 0) {
        auto out = order;
        out.insert(at < 0 ? out.end() : out.begin() + at + 1, name);
        return out;
    }
    return moved(order, {name}, at < 0 ? (int) order.size() : at + 1);
}

struct Extent {
    int top = 0, bottom = 0;
    bool shown() const { return bottom > top; }
};

inline int dropBefore(const std::vector<Extent>& rows, int y) {
    int before = 0;
    for (int i = 0; i < (int) rows.size(); ++i) {
        const auto& r = rows[(size_t) i];
        if (!r.shown()) continue;
        if (y < (r.top + r.bottom) / 2) return i;
        before = i + 1;
    }
    return before;
}

inline bool wouldMove(const std::vector<std::string>& order, const std::set<std::string>& moving,
                      int before) {
    return moved(order, moving, before) != order;
}

}
