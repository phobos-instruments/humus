// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace hum {

inline constexpr const char* kParamBankChannel = "param-bank";

using BankValues = std::vector<std::pair<std::string, long long>>;

inline std::string encodeBankValues(const BankValues& values) {
    std::string m;
    for (const auto& [name, v] : values) {
        if (!m.empty()) m += ' ';
        m += name + '=' + std::to_string(v);
    }
    return m;
}

inline int numberedSlot(const std::string& name, const std::string& prefix) {
    if (prefix.empty() || name.size() <= prefix.size() || name.compare(0, prefix.size(), prefix) != 0) return -1;
    return std::atoi(name.c_str() + prefix.size()) - 1;
}

inline BankValues decodeBankValues(const std::string& m) {
    BankValues out;
    size_t at = 0;
    while (at < m.size()) {
        const size_t end = std::min(m.find(' ', at), m.size());
        const size_t eq = m.find('=', at);
        if (eq != std::string::npos && eq > at && eq < end)
            out.emplace_back(m.substr(at, eq - at), std::strtoll(m.c_str() + eq + 1, nullptr, 10));
        at = end + 1;
    }
    return out;
}

}
