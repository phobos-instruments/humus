// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <vector>

#include "hum/LayoutSpec.h"

namespace hum {

class LayoutCondition {
public:
    struct Clause {
        bool invert = false;
        bool flag = false;
        std::string name;
        bool equals = false;
        double value = 0.0;
        std::string option;
    };

    static LayoutCondition parse(const std::string& expr, const LayoutSpec& spec,
                                 std::vector<std::string>* problems = nullptr);

    bool empty() const { return anyOf_.empty(); }
    std::vector<std::string> params() const;
    std::vector<Clause> clauses() const;

    template <class ValueOf, class FlagOf>
    bool holds(ValueOf&& valueOf, FlagOf&& flagOf) const {
        for (const auto& group : anyOf_) {
            bool all = true;
            for (const auto& c : group) {
                const bool on = c.flag     ? flagOf(c.name)
                                : c.equals ? std::abs(valueOf(c.name) - c.value) < 0.5
                                           : valueOf(c.name) >= 0.5;
                if (on == c.invert) { all = false; break; }
            }
            if (all) return true;
        }
        return false;
    }

private:
    std::vector<std::vector<Clause>> anyOf_;
};

}
