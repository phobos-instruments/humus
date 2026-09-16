// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/app/FuzzyMatch.h"

namespace hum {
namespace picker {

using Groups = std::vector<std::pair<std::string, CategoryGroups>>;
using Path = std::vector<std::string>;

struct Folder {
    std::string label;
    int folderCount = 0;
    int classCount = 0;
};

struct Leaf {
    std::string cls;
    std::string display;
    std::string tag;
};

struct Level {
    std::vector<Folder> folders;
    std::vector<Leaf> leaves;
};

inline std::string leafTag(const std::string& origin, const std::string& category) {
    return origin + " \xe2\x80\xba " + category;   // utf8-ok: data ("origin › category")
}

inline Level childrenAt(const Groups& groups, const Path& path) {
    Level out;
    if (path.empty()) {
        for (const auto& o : groups) {
            Folder f;
            f.label = o.first;
            std::vector<std::string> firsts;
            for (const auto& g : o.second) {
                const auto segs = categorySegments(g.first);
                f.classCount += (int) g.second.size();
                if (!segs.empty()
                    && std::find(firsts.begin(), firsts.end(), segs[0]) == firsts.end())
                    firsts.push_back(segs[0]);
            }
            f.folderCount = (int) firsts.size();
            out.folders.push_back(std::move(f));
        }
        return out;
    }
    const auto origin = std::find_if(groups.begin(), groups.end(),
                                     [&](const auto& o) { return o.first == path[0]; });
    if (origin == groups.end()) return out;
    const size_t depth = path.size() - 1;

    for (const auto& g : origin->second) {
        const auto segs = categorySegments(g.first);
        if (segs.size() < depth) continue;
        bool under = true;
        for (size_t d = 0; d < depth && under; ++d) under = segs[d] == path[d + 1];
        if (!under) continue;

        if (segs.size() == depth) {
            for (const auto& cls : g.second)
                out.leaves.push_back({cls, parseClassString(cls).display,
                                      leafTag(origin->first, g.first)});
            continue;
        }
        const std::string& next = segs[depth];
        auto it = std::find_if(out.folders.begin(), out.folders.end(),
                               [&](const Folder& f) { return f.label == next; });
        if (it == out.folders.end()) {
            out.folders.push_back({next, 0, 0});
            it = out.folders.end() - 1;
        }
        it->classCount += (int) g.second.size();
    }
    for (auto& f : out.folders) {
        Path deeper = path;
        deeper.push_back(f.label);
        const Level l = childrenAt(groups, deeper);
        f.folderCount = (int) l.folders.size();
    }
    return out;
}

inline std::vector<std::string> breadcrumb(const Path& path) {
    std::vector<std::string> out{"All"};
    out.insert(out.end(), path.begin(), path.end());
    return out;
}

inline Path parentOf(Path path) {
    if (!path.empty()) path.pop_back();
    return path;
}

using Describe = std::function<std::string(const std::string& cls)>;

inline std::vector<Leaf> searchScoped(const Groups& groups, const Path& path,
                                      const std::string& query, const Describe& describe = {}) {
    struct Scored { Leaf leaf; int score; };
    std::vector<Scored> hits;
    for (const auto& o : groups) {
        if (!path.empty() && o.first != path[0]) continue;
        for (const auto& g : o.second) {
            const auto segs = categorySegments(g.first);
            bool under = segs.size() + 1 >= path.size();
            for (size_t d = 0; d + 1 < path.size() && under; ++d)
                under = segs[d] == path[d + 1];
            if (!under) continue;
            const std::string tag = leafTag(o.first, g.first);
            for (const auto& cls : g.second) {
                const std::string display = parseClassString(cls).display;
                const int score = describe
                    ? fuzzyScoreNameMetaText(query, display, tag, describe(cls))
                    : fuzzyScoreNameThenMeta(query, display, tag);
                if (score < 0) continue;
                hits.push_back({{cls, display, tag}, score});
            }
        }
    }
    std::stable_sort(hits.begin(), hits.end(), [](const Scored& a, const Scored& b) {
        return a.score != b.score ? a.score > b.score : a.leaf.display < b.leaf.display;
    });
    std::vector<Leaf> out;
    out.reserve(hits.size());
    for (auto& h : hits) out.push_back(std::move(h.leaf));
    return out;
}

inline std::string firstSentence(const std::string& text) {
    std::string t;
    for (const char c : text) t += (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
    while (!t.empty() && t.front() == ' ') t.erase(t.begin());
    for (size_t i = 0; i + 1 < t.size(); ++i)
        if ((t[i] == '.' || t[i] == '!' || t[i] == '?') && t[i + 1] == ' ') {
            const bool abbreviation = i >= 2 && std::isupper((unsigned char) t[i - 1]) && t[i - 2] == ' ';
            if (!abbreviation) { t.erase(i + 1); break; }
        }
    while (!t.empty() && t.back() == ' ') t.pop_back();
    return t;
}

inline std::string monogram(const std::string& display) {
    std::string out;
    for (size_t i = 0; i < display.size() && out.size() < 2; ++i) {
        const unsigned char c = (unsigned char) display[i];
        const unsigned char p = i > 0 ? (unsigned char) display[i - 1] : ' ';
        const bool significant = i == 0 || p == ' ' || std::isupper(c)
            || (std::isdigit(c) && !std::isdigit(p));
        if (significant && std::isalnum(c)) out += (char) std::toupper(c);
    }
    return out;
}

inline int tintIndex(const std::string& cls, int paletteSize) {
    unsigned int h = 2166136261u;
    for (const char c : cls) h = (h ^ (unsigned char) c) * 16777619u;
    return paletteSize > 0 ? (int) (h % (unsigned int) paletteSize) : 0;
}

}
}
