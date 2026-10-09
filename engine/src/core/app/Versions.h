// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace hum {

inline int versionPart(const std::string& part) {
    if (part.empty() || part.find_first_not_of("0123456789") != std::string::npos) return 0;
    int n = 0;
    for (const char c : part) n = std::min(n * 10 + (c - '0'), 1000000);
    return n;
}

inline std::vector<int> versionParts(const std::string& version) {
    std::vector<int> parts;
    for (std::size_t start = 0;;) {
        const std::size_t dot = version.find('.', start);
        parts.push_back(versionPart(version.substr(start, dot == std::string::npos ? std::string::npos : dot - start)));
        if (dot == std::string::npos) return parts;
        start = dot + 1;
    }
}

inline int compareVersions(const std::string& a, const std::string& b) {
    const auto pa = versionParts(a), pb = versionParts(b);
    for (std::size_t i = 0; i < std::max(pa.size(), pb.size()); ++i) {
        const int x = i < pa.size() ? pa[i] : 0, y = i < pb.size() ? pb[i] : 0;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

}
