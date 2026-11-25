// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {

struct DawMacroTarget {
    std::string organism, param;
    double from = 0.0, to = 1.0;

    bool is(const std::string& o, const std::string& p) const { return organism == o && param == p; }
    bool operator==(const DawMacroTarget& other) const {
        return is(other.organism, other.param) && from == other.from && to == other.to;
    }
};

}
