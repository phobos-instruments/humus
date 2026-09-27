// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/FileIndex.h"

#include "core/project/ProjectFolder.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>

namespace hum::browser {

std::string normalTag(const std::string& raw) {
    std::string out;
    bool gap = false;
    for (const char c : raw) {
        const auto u = (unsigned char) c;
        if (std::isspace(u) || c == '#' || c == ',') { gap = !out.empty(); continue; }
        if (gap) out += '-';
        gap = false;
        out += (char) std::tolower(u);
    }
    return out;
}

const Entry* FileIndex::find(const std::string& path) const {
    const auto it = entries_.find(path);
    return it == entries_.end() ? nullptr : &it->second;
}

Entry* FileIndex::at(const std::string& path) {
    const auto it = entries_.find(path);
    return it == entries_.end() ? nullptr : &it->second;
}

Entry& FileIndex::ensure(const std::string& path, std::int64_t now) {
    auto [it, fresh] = entries_.try_emplace(path);
    if (fresh) {
        it->second.path = path;
        it->second.kind = kindOfFile(path);
        it->second.added = now;
        dirty_ = true;
    }
    return it->second;
}

void FileIndex::observe(const std::string& path, std::int64_t size, std::int64_t modified, std::int64_t now,
                        Kind kind, std::int64_t created) {
    auto& e = ensure(path, now);
    if (created > 0 && e.created != created) {
        e.created = created;
        dirty_ = true;
    }
    if (kind != Kind::Other && e.kind != kind) {
        e.kind = kind;
        dirty_ = true;
    }
    if (e.size == size && e.modified == modified) return;
    if (e.modified != 0 && e.modified != modified) e.facts = {};
    e.size = size;
    e.modified = modified;
    dirty_ = true;
}

void FileIndex::setFacts(const std::string& path, const Facts& facts) {
    if (auto* e = at(path)) {
        e->facts = facts;
        dirty_ = true;
    }
}

bool FileIndex::forget(const std::string& path) {
    if (entries_.erase(path) == 0) return false;
    for (auto& c : collections_)
        c.paths.erase(std::remove(c.paths.begin(), c.paths.end(), path), c.paths.end());
    dirty_ = true;
    return true;
}

void FileIndex::setRating(const std::string& path, int stars, std::int64_t now) {
    auto& e = ensure(path, now);
    e.rating = std::clamp(stars, 0, kMaxRating);
    dirty_ = true;
}

void FileIndex::setFavourite(const std::string& path, bool on, std::int64_t now) {
    auto& e = ensure(path, now);
    e.favourite = on;
    dirty_ = true;
}

bool FileIndex::addTag(const std::string& path, const std::string& tag, std::int64_t now) {
    const auto t = normalTag(tag);
    if (t.empty()) return false;
    auto& e = ensure(path, now);
    if (std::find(e.tags.begin(), e.tags.end(), t) != e.tags.end()) return false;
    e.tags.push_back(t);
    dirty_ = true;
    return true;
}

bool FileIndex::removeTag(const std::string& path, const std::string& tag) {
    auto* e = at(path);
    if (e == nullptr) return false;
    const auto t = normalTag(tag);
    const auto it = std::find(e->tags.begin(), e->tags.end(), t);
    if (it == e->tags.end()) return false;
    e->tags.erase(it);
    dirty_ = true;
    return true;
}

std::vector<std::string> FileIndex::tagsInUse() const {
    std::set<std::string> all;
    for (const auto& [path, e] : entries_) all.insert(e.tags.begin(), e.tags.end());
    return {all.begin(), all.end()};
}

void FileIndex::noteUsed(const std::string& path, std::int64_t now) {
    ensure(path, now).lastUsed = now;
    dirty_ = true;
}

void FileIndex::forgetUse(const std::string& path) {
    if (auto* e = at(path); e != nullptr && e->lastUsed > 0) {
        e->lastUsed = 0;
        dirty_ = true;
    }
}

void FileIndex::carryAnnotations(const std::string& from, const std::string& to, std::int64_t now) {
    const auto* src = find(from);
    if (src == nullptr || from == to) return;
    const auto rating = src->rating;
    const bool favourite = src->favourite;
    const auto tags = src->tags;
    const auto facts = src->facts;
    auto& dst = ensure(to, now);
    dst.rating = std::max(dst.rating, rating);
    dst.favourite = dst.favourite || favourite;
    for (const auto& t : tags)
        if (std::find(dst.tags.begin(), dst.tags.end(), t) == dst.tags.end()) dst.tags.push_back(t);
    if (!dst.facts.probed && facts.probed) dst.facts = facts;
    for (auto& c : collections_)
        if (std::find(c.paths.begin(), c.paths.end(), from) != c.paths.end()
            && std::find(c.paths.begin(), c.paths.end(), to) == c.paths.end())
            c.paths.push_back(to);
    dirty_ = true;
}

std::vector<const Entry*> FileIndex::recent(int most) const {
    std::vector<const Entry*> out;
    for (const auto& [path, e] : entries_)
        if (e.lastUsed > 0) out.push_back(&e);
    std::sort(out.begin(), out.end(), [](const Entry* a, const Entry* b) { return a->lastUsed > b->lastUsed; });
    if ((int) out.size() > most) out.resize((size_t) std::max(0, most));
    return out;
}

std::vector<const Entry*> FileIndex::favourites() const {
    std::vector<const Entry*> out;
    for (const auto& [path, e] : entries_)
        if (e.favourite) out.push_back(&e);
    return out;
}

bool FileIndex::addCollection(const std::string& name) {
    if (name.empty() || collection(name) != nullptr) return false;
    collections_.push_back({name, {}});
    dirty_ = true;
    return true;
}

bool FileIndex::renameCollection(const std::string& from, const std::string& to) {
    if (to.empty() || collection(to) != nullptr) return false;
    for (auto& c : collections_)
        if (c.name == from) {
            c.name = to;
            dirty_ = true;
            return true;
        }
    return false;
}

bool FileIndex::removeCollection(const std::string& name) {
    const auto before = collections_.size();
    collections_.erase(std::remove_if(collections_.begin(), collections_.end(),
                                      [&](const Collection& c) { return c.name == name; }),
                       collections_.end());
    dirty_ = dirty_ || collections_.size() != before;
    return collections_.size() != before;
}

bool FileIndex::addToCollection(const std::string& name, const std::string& path) {
    for (auto& c : collections_) {
        if (c.name != name) continue;
        if (std::find(c.paths.begin(), c.paths.end(), path) != c.paths.end()) return false;
        c.paths.push_back(path);
        dirty_ = true;
        return true;
    }
    return false;
}

bool FileIndex::removeFromCollection(const std::string& name, const std::string& path) {
    for (auto& c : collections_) {
        if (c.name != name) continue;
        const auto it = std::find(c.paths.begin(), c.paths.end(), path);
        if (it == c.paths.end()) return false;
        c.paths.erase(it);
        dirty_ = true;
        return true;
    }
    return false;
}

const Collection* FileIndex::collection(const std::string& name) const {
    for (const auto& c : collections_)
        if (c.name == name) return &c;
    return nullptr;
}

std::vector<std::string> FileIndex::collectionsHolding(const std::string& path) const {
    std::vector<std::string> out;
    for (const auto& c : collections_)
        if (std::find(c.paths.begin(), c.paths.end(), path) != c.paths.end()) out.push_back(c.name);
    return out;
}

bool FileIndex::watch(const std::string& dir) {
    if (dir.empty() || std::find(watched_.begin(), watched_.end(), dir) != watched_.end()) return false;
    watched_.push_back(dir);
    dirty_ = true;
    return true;
}

bool FileIndex::unwatch(const std::string& dir) {
    const auto it = std::find(watched_.begin(), watched_.end(), dir);
    if (it == watched_.end()) return false;
    watched_.erase(it);
    dirty_ = true;
    return true;
}

std::string FileIndex::relocate(const std::string& foundPath, std::int64_t size,
                                const std::vector<std::string>& missing) {
    if (find(foundPath) != nullptr && find(foundPath)->annotated()) return {};
    const auto name = fileName(foundPath);
    std::string match;
    for (const auto& gone : missing) {
        const auto* e = find(gone);
        if (e == nullptr || !e->annotated() || e->size != size || fileName(gone) != name) continue;
        if (!match.empty()) return {};
        match = gone;
    }
    if (!match.empty()) movePath(match, foundPath);
    return match;
}

int FileIndex::forgetOutside(const std::vector<std::string>& places) {
    auto inside = [&](const std::string& path) {
        for (const auto& p : places) {
            if (p.empty()) continue;
            if (path == p || (path.size() > p.size() && path.compare(0, p.size(), p) == 0
                              && (p.back() == '/' || path[p.size()] == '/' || path[p.size()] == '\\')))
                return true;
        }
        return false;
    };
    std::vector<std::string> gone;
    for (const auto& [path, e] : entries_)
        if (!e.annotated() && !inside(path)) gone.push_back(path);
    for (const auto& path : gone) forget(path);
    return (int) gone.size();
}

int FileIndex::importRecents(const std::vector<std::string>& newestFirst, std::int64_t now) {
    for (const auto& [path, e] : entries_)
        if (e.lastUsed > 0 && (e.kind == Kind::Project || e.kind == Kind::Patch)) return 0;
    int n = 0;
    for (const auto& path : newestFirst) {
        const auto root = project::rootOf(path);
        const auto& target = root.empty() ? path : root;
        auto& e = ensure(target, now);
        if (!root.empty()) e.kind = Kind::Project;
        e.lastUsed = now - n;
        ++n;
    }
    dirty_ = dirty_ || n > 0;
    return n;
}

int FileIndex::forgetRecentPatches() {
    int n = 0;
    for (auto& [path, e] : entries_)
        if (e.lastUsed > 0 && (e.kind == Kind::Project || e.kind == Kind::Patch)) {
            e.lastUsed = 0;
            ++n;
        }
    dirty_ = dirty_ || n > 0;
    return n;
}

void FileIndex::movePath(const std::string& from, const std::string& to) {
    const auto it = entries_.find(from);
    if (it == entries_.end() || from == to) return;
    Entry moved = it->second;
    entries_.erase(it);
    moved.path = to;
    entries_[to] = std::move(moved);
    for (auto& c : collections_)
        std::replace(c.paths.begin(), c.paths.end(), from, to);
    dirty_ = true;
}

}
