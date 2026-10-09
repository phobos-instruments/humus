// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>
#include <vector>

namespace hum {

struct PoEntry {
    std::string context;
    std::string id;
    std::string idPlural;
    std::vector<std::string> forms;
    bool fuzzy = false;

    bool isPlural() const { return !idPlural.empty(); }
    const std::string& key() const { return context.empty() ? id : context; }
};

struct PoFile {
    std::map<std::string, std::string> header;
    std::vector<PoEntry> entries;
};

PoFile parsePo(const std::string& text);

}
