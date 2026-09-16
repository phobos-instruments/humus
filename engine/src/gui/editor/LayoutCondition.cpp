// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/LayoutCondition.h"

#include <cstdlib>
#include <limits>

#include "hum/Number.h"

namespace hum {

namespace {

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    for (size_t at = 0;;) {
        const auto next = s.find(sep, at);
        out.push_back(s.substr(at, next == std::string::npos ? next : next - at));
        if (next == std::string::npos) return out;
        at = next + 1;
    }
}

bool isNumber(const std::string& s) {
    const char* end = nullptr;
    scanDouble(s.c_str(), &end);
    return !s.empty() && end == s.c_str() + s.size();
}

bool optionValue(const LayoutSpec& spec, const std::string& param, const std::string& option,
                 double& out) {
    for (const auto& c : spec.controls) {
        if (c.param != param) continue;
        for (size_t i = 0; i < c.options.size(); ++i)
            if (c.options[i] == option) {
                out = (double) i + std::atoi(c.extraOr("first", "0").c_str());
                return true;
            }
    }
    return false;
}

}

LayoutCondition LayoutCondition::parse(const std::string& expr, const LayoutSpec& spec,
                                       std::vector<std::string>* problems) {
    LayoutCondition cond;
    if (expr.empty()) return cond;
    const auto problem = [&](const std::string& what) {
        if (problems != nullptr) problems->push_back("'" + expr + "': " + what);
    };
    for (const auto& alternative : split(expr, '|')) {
        std::vector<Clause> group;
        for (const auto& text : split(alternative, '&')) {
            if (text.empty()) continue;
            Clause c;
            c.invert = text[0] == '!';
            std::string gate = c.invert ? text.substr(1) : text;
            if (!gate.empty() && gate[0] == '@') {
                c.flag = true;
                c.name = gate.substr(1);
            } else if (const auto eq = gate.find('='); eq != std::string::npos) {
                c.equals = true;
                c.name = gate.substr(0, eq);
                const auto want = gate.substr(eq + 1);
                if (isNumber(want)) {
                    c.value = scanDouble(want.c_str());
                } else {
                    c.option = want;
                    if (!optionValue(spec, c.name, want, c.value)) {
                        problem("no control for " + c.name + " lists an option '" + want + "'");
                        c.value = std::numeric_limits<double>::quiet_NaN();
                    }
                }
            } else {
                c.name = gate;
            }
            if (c.name.empty()) problem("a clause names nothing");
            group.push_back(std::move(c));
        }
        if (group.empty()) problem("an empty alternative always holds");
        cond.anyOf_.push_back(std::move(group));
    }
    return cond;
}

std::vector<LayoutCondition::Clause> LayoutCondition::clauses() const {
    std::vector<Clause> out;
    for (const auto& group : anyOf_) out.insert(out.end(), group.begin(), group.end());
    return out;
}

std::vector<std::string> LayoutCondition::params() const {
    std::vector<std::string> out;
    for (const auto& c : clauses())
        if (!c.flag) out.push_back(c.name);
    return out;
}

}
