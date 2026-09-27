// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cctype>
#include <functional>
#include <string>
#include <vector>

namespace hum::files {

struct FilePick {
    std::string title;
    std::string startDir;
    std::string patterns;
    bool save = false;
    bool directories = false;
    bool multiple = false;
    std::string current;
    std::string kind;
};

using Picked = std::function<void(const std::vector<std::string>& paths)>;

class FilePicker {
public:
    virtual ~FilePicker() = default;
    virtual void pick(const FilePick& request, Picked done) = 0;
    virtual bool open() const = 0;
};

inline std::string fileNameOf(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

inline std::string stemOf(const std::string& path) {
    const auto name = fileNameOf(path);
    const auto dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

inline std::string parentOf(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    if (slash == std::string::npos) return {};
    return slash == 0 ? path.substr(0, 1) : path.substr(0, slash);
}

inline bool hasExtension(const std::string& path) { return fileNameOf(path).find('.') != std::string::npos; }

inline std::string stripScheme(const std::string& uri) { return uri.rfind("file://", 0) == 0 ? uri.substr(7) : uri; }

inline bool wildcardMatch(const char* pattern, const char* name) {
    for (;; ++pattern, ++name) {
        if (*pattern == '*') {
            while (pattern[1] == '*') ++pattern;
            if (pattern[1] == 0) return true;
            for (; *name != 0; ++name)
                if (wildcardMatch(pattern + 1, name)) return true;
            return false;
        }
        if (*name == 0) return *pattern == 0;
        if (*pattern != '?' && std::tolower((unsigned char) *pattern) != std::tolower((unsigned char) *name))
            return false;
    }
}

inline bool matchesPatterns(const std::string& fileName, const std::string& patterns) {
    size_t from = 0;
    while (from <= patterns.size()) {
        auto to = patterns.find(';', from);
        if (to == std::string::npos) to = patterns.size();
        auto pat = patterns.substr(from, to - from);
        while (!pat.empty() && std::isspace((unsigned char) pat.front())) pat.erase(pat.begin());
        while (!pat.empty() && std::isspace((unsigned char) pat.back())) pat.pop_back();
        if (!pat.empty() && wildcardMatch(pat.c_str(), fileName.c_str())) return true;
        from = to + 1;
    }
    return false;
}

}
