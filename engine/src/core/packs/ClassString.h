// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>

namespace hum {

struct ClassId {
    std::string display;
    std::string kind = "native";
    std::map<std::string, std::string> query;

    std::string get(const std::string& key) const {
        auto it = query.find(key);
        return it == query.end() ? std::string{} : it->second;
    }
};

inline ClassId parseClassString(const std::string& raw) {
    ClassId id;
    const auto q = raw.find('?');
    id.display = raw.substr(0, q);
    if (q == std::string::npos) return id;

    const std::string rest = raw.substr(q + 1);
    size_t pos = 0;
    while (pos <= rest.size()) {
        const auto amp = rest.find('&', pos);
        const std::string pair =
            rest.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
        const auto eq = pair.find('=');
        if (eq != std::string::npos) id.query[pair.substr(0, eq)] = pair.substr(eq + 1);
        else if (!pair.empty())      id.query[pair] = "";
        if (amp == std::string::npos) break;
        pos = amp + 1;
    }

    const auto type = id.get("type");
    if (type == "au")        id.kind = "au";
    else if (type == "vst3") id.kind = "vst3";
    else if (type == "lv2")  id.kind = "lv2";
    return id;
}

inline bool isPluginKind(const std::string& kind) {
    return kind == "vst3" || kind == "lv2" || kind == "au";
}

}
