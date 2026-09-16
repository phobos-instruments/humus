// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <sstream>
#include <string>

#include "hum/ParseInt.h"

namespace hum {

struct SliceEdit {
    int pitch = 0;
    int gainPct = 100;
    bool reverse = false;
    bool isDefault() const { return pitch == 0 && gainPct == 100 && !reverse; }
};

inline bool parseSignedInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    const bool negative = s[0] == '-';
    const int magnitude = parseBoundedInt(negative ? s.substr(1) : s);
    if (magnitude < 0) return false;
    out = negative ? -magnitude : magnitude;
    return true;
}

inline std::map<int, SliceEdit> parseSliceEdits(const std::string& s) {
    std::map<int, SliceEdit> out;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ';')) {
        if (tok.empty()) continue;
        std::istringstream fields(tok);
        std::string field;
        int v[4] = {0, 0, 100, 0};
        int n = 0;
        bool ok = true;
        while (ok && n < 4 && std::getline(fields, field, ':')) ok = parseSignedInt(field, v[n++]);
        if (!ok || n != 4) continue;
        SliceEdit e{v[1], v[2], v[3] != 0};
        if (v[0] >= 0 && !e.isDefault()) out[v[0]] = e;
    }
    return out;
}

inline std::map<int, std::string> parseSlicePins(const std::string& s) {
    std::map<int, std::string> out;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ';')) {
        const auto colon = tok.find(':');
        if (colon == std::string::npos || colon == 0 || colon + 1 >= tok.size()) continue;
        if (const int index = parseBoundedInt(tok.substr(0, colon)); index >= 0)
            out[index] = tok.substr(colon + 1);
    }
    return out;
}

inline std::string encodeSlicePins(const std::map<int, std::string>& pins) {
    std::string out;
    for (const auto& [index, payload] : pins) {
        if (payload.empty()) continue;
        out += std::to_string(index) + ":" + payload + ";";
    }
    return out;
}

inline std::string encodeSliceEdits(const std::map<int, SliceEdit>& m) {
    std::string out;
    for (const auto& [idx, e] : m) {
        if (e.isDefault()) continue;
        out += std::to_string(idx) + ":" + std::to_string(e.pitch) + ":"
             + std::to_string(e.gainPct) + ":" + (e.reverse ? "1" : "0") + ";";
    }
    return out;
}

}
