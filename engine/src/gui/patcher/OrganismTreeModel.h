// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace hum {
inline constexpr const char* kClassDragPrefix = "humus-class:";
}

namespace hum::organismtree {

struct Leaf {
    std::string cls, display, badge;
};

struct Folder {
    std::string name;
    std::vector<Folder> folders;
    std::vector<Leaf> leaves;
};

inline Folder& childOf(Folder& parent, const std::string& name) {
    for (auto& f : parent.folders)
        if (f.name == name) return f;
    parent.folders.push_back(Folder{name, {}, {}});
    return parent.folders.back();
}

inline void place(Folder& root, const std::vector<std::string>& path, Leaf leaf) {
    Folder* at = &root;
    for (const auto& name : path) at = &childOf(*at, name);
    at->leaves.push_back(std::move(leaf));
}

inline std::string folded(const std::string& s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return (char) std::tolower(c); });
    return out;
}

inline bool matches(const std::string& text, const std::string& foldedQuery) {
    return folded(text).find(foldedQuery) != std::string::npos;
}

inline bool hasLeaves(const Folder& f) {
    if (!f.leaves.empty()) return true;
    return std::any_of(f.folders.begin(), f.folders.end(), [](const Folder& c) { return hasLeaves(c); });
}

inline Folder filteredFolder(const Folder& folder, const std::string& foldedQuery) {
    if (matches(folder.name, foldedQuery)) return folder;
    Folder kept{folder.name, {}, {}};
    for (const auto& child : folder.folders)
        if (auto f = filteredFolder(child, foldedQuery); hasLeaves(f)) kept.folders.push_back(std::move(f));
    for (const auto& leaf : folder.leaves)
        if (matches(leaf.display, foldedQuery)) kept.leaves.push_back(leaf);
    return kept;
}

inline std::vector<Folder> filtered(const std::vector<Folder>& all, const std::string& query) {
    const auto q = folded(query);
    if (q.empty()) return all;
    std::vector<Folder> out;
    for (const auto& section : all)
        if (auto f = filteredFolder(section, q); hasLeaves(f)) out.push_back(std::move(f));
    return out;
}

inline int leafCount(const Folder& f) {
    int n = (int) f.leaves.size();
    for (const auto& c : f.folders) n += leafCount(c);
    return n;
}

inline int leafCount(const std::vector<Folder>& sections) {
    int n = 0;
    for (const auto& s : sections) n += leafCount(s);
    return n;
}

}
