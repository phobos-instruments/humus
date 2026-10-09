// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/PageLanguage.h"

#include <cstddef>

namespace hum {

namespace {

constexpr const char* kFence = "---";

std::string trimmedLine(const std::string& page, std::size_t from, std::size_t to) {
    while (from < to && (page[from] == ' ' || page[from] == '\t')) ++from;
    while (to > from && (page[to - 1] == ' ' || page[to - 1] == '\t' || page[to - 1] == '\r')) --to;
    return page.substr(from, to - from);
}

std::size_t lineEnd(const std::string& page, std::size_t from) {
    const auto end = page.find('\n', from);
    return end == std::string::npos ? page.size() : end;
}

}

std::string& pageLanguage() {
    static std::string code;
    return code;
}

std::vector<std::string> pageCandidates(const std::string& path) {
    std::vector<std::string> out;
    if (!pageLanguage().empty()) out.push_back("translations/" + pageLanguage() + "/" + path);
    out.push_back(path);
    return out;
}

std::vector<std::string> embeddedCandidates(const std::string& stem) {
    std::vector<std::string> out;
    if (!pageLanguage().empty()) out.push_back(stem + "." + pageLanguage() + ".md");
    out.push_back(stem + ".md");
    return out;
}

std::string withoutFrontMatter(const std::string& page) {
    std::size_t at = page.rfind("\xef\xbb\xbf", 0) == 0 ? 3 : 0;  // utf8-ok
    if (trimmedLine(page, at, lineEnd(page, at)) != kFence) return page;
    for (std::size_t line = lineEnd(page, at) + 1; line < page.size(); line = lineEnd(page, line) + 1) {
        if (trimmedLine(page, line, lineEnd(page, line)) != kFence) continue;
        std::size_t body = lineEnd(page, line) + 1;
        while (body < page.size() && (page[body] == '\n' || page[body] == '\r')) ++body;
        return body < page.size() ? page.substr(body) : std::string();
    }
    return page;
}

}
