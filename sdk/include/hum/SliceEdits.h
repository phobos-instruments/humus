#pragma once
#include <cstdio>
#include <map>
#include <sstream>
#include <string>

namespace hum {

struct SliceEdit {
    int pitch = 0;
    int gainPct = 100;
    bool reverse = false;
    bool isDefault() const { return pitch == 0 && gainPct == 100 && !reverse; }
};

inline std::map<int, SliceEdit> parseSliceEdits(const std::string& s) {
    std::map<int, SliceEdit> out;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ';')) {
        if (tok.empty()) continue;
        int idx = 0, p = 0, g = 100, r = 0;
        if (std::sscanf(tok.c_str(), "%d:%d:%d:%d", &idx, &p, &g, &r) == 4) {
            SliceEdit e{p, g, r != 0};
            if (idx >= 0 && !e.isDefault()) out[idx] = e;
        }
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
