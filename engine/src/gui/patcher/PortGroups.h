// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "hum/caps/Graph.h"

namespace hum::portgroups {

struct Side {
    std::vector<std::string> names;
    std::vector<int> breaksBefore;

    int count() const { return (int) names.size(); }
    int breaks() const { return breaksBefore.empty() ? 0 : breaksBefore.back(); }
    int breaksAt(int port) const {
        if (breaksBefore.empty()) return 0;
        return breaksBefore[(size_t) (port < 0 ? 0 : port >= count() ? count() - 1 : port)];
    }
    std::string name(int port) const { return port >= 0 && port < count() ? names[(size_t) port] : std::string(); }
};

inline std::string groupOf(const std::string& name) {
    const auto space = name.find_last_of(' ');
    return space == std::string::npos ? name : name.substr(0, space);
}

inline Side describe(const PortNames* ports, int count, bool outlets) {
    Side side;
    if (ports == nullptr || count <= 0) return side;
    int breaks = 0;
    for (int i = 0; i < count; ++i) {
        auto name = outlets ? ports->outletName(i) : ports->inletName(i);
        if (i > 0 && groupOf(name) != groupOf(side.names.back())) ++breaks;
        side.names.push_back(std::move(name));
        side.breaksBefore.push_back(breaks);
    }
    return side;
}

}
