// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "core/packs/PackManifest.h"
#include "core/params/ParamSchema.h"
#include "hum/ParamBanks.h"
#include "hum/Parameter.h"
#include "hum/Pattern.h"

namespace hum::parambanks {

inline bool banked(const OrganismClassManifest& m, const std::string& param) {
    for (const auto& prefix : m.bankedPrefixes)
        if (!prefix.empty() && param.rfind(prefix, 0) == 0) return true;
    return false;
}

inline double currentValue(const std::vector<Parameter>& properties, const ParamDesc& d) {
    for (const auto& p : properties)
        if (p.name == d.name) return p.value;
    return d.def;
}

inline BankValues shownValues(const OrganismClassManifest& m, const std::vector<ParamDesc>& schema,
                              const std::vector<Parameter>& properties) {
    BankValues shown;
    for (const auto& d : schema)
        if ((d.isInt || d.isEnum || d.isBool) && banked(m, d.name))
            shown.emplace_back(d.name, std::llround(currentValue(properties, d)));
    return shown;
}

inline int shownBank(const OrganismClassManifest& m, const std::vector<ParamDesc>& schema,
                     const std::vector<Parameter>& properties) {
    for (const auto& d : schema)
        if (d.name == m.bankSelect) return (int) std::lround(currentValue(properties, d));
    return -1;
}

inline PatternChannel& bankChannel(Pattern& pattern, int bank) {
    int seen = 0;
    for (auto& ch : pattern.channels)
        if (ch.type == kParamBankChannel && seen++ == bank) return ch;
    for (; seen < bank; ++seen) {
        PatternChannel empty;
        empty.type = kParamBankChannel;
        pattern.channels.push_back(empty);
    }
    PatternChannel ch;
    ch.type = kParamBankChannel;
    pattern.channels.push_back(ch);
    return pattern.channels.back();
}

inline BankValues switchBank(Pattern& pattern, const BankValues& shown, int from, int to) {
    bankChannel(pattern, std::max(from, to));
    pattern.present = true;
    auto& target = bankChannel(pattern, to);
    BankValues load = decodeBankValues(target.matrix);
    target.matrix.clear();
    bankChannel(pattern, from).matrix = encodeBankValues(shown);
    return load;
}

}
