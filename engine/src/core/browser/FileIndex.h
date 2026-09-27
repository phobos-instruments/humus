// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "core/browser/BrowserEntry.h"

namespace hum::browser {

std::string normalTag(const std::string& raw);

class FileIndex {
public:
    static constexpr int kRecentMost = 50;

    const Entry* find(const std::string& path) const;
    Entry& ensure(const std::string& path, std::int64_t now);
    void observe(const std::string& path, std::int64_t size, std::int64_t modified, std::int64_t now,
                 Kind kind = Kind::Other, std::int64_t created = 0);
    void setFacts(const std::string& path, const Facts& facts);
    bool forget(const std::string& path);

    void setRating(const std::string& path, int stars, std::int64_t now);
    void setFavourite(const std::string& path, bool on, std::int64_t now);
    bool addTag(const std::string& path, const std::string& tag, std::int64_t now);
    bool removeTag(const std::string& path, const std::string& tag);
    std::vector<std::string> tagsInUse() const;

    void noteUsed(const std::string& path, std::int64_t now);
    void forgetUse(const std::string& path);
    void carryAnnotations(const std::string& from, const std::string& to, std::int64_t now);
    std::vector<const Entry*> recent(int most = kRecentMost) const;
    std::vector<const Entry*> favourites() const;

    bool addCollection(const std::string& name);
    bool renameCollection(const std::string& from, const std::string& to);
    bool removeCollection(const std::string& name);
    bool addToCollection(const std::string& name, const std::string& path);
    bool removeFromCollection(const std::string& name, const std::string& path);
    const Collection* collection(const std::string& name) const;
    const std::vector<Collection>& collections() const { return collections_; }
    std::vector<std::string> collectionsHolding(const std::string& path) const;

    bool watch(const std::string& dir);
    bool unwatch(const std::string& dir);
    const std::vector<std::string>& watched() const { return watched_; }

    std::string relocate(const std::string& foundPath, std::int64_t size,
                         const std::vector<std::string>& missing);
    void movePath(const std::string& from, const std::string& to);
    int forgetOutside(const std::vector<std::string>& places);
    int importRecents(const std::vector<std::string>& newestFirst, std::int64_t now);
    int forgetRecentPatches();

    const std::map<std::string, Entry>& entries() const { return entries_; }
    bool dirty() const { return dirty_; }
    void markClean() { dirty_ = false; }

private:
    Entry* at(const std::string& path);

    std::map<std::string, Entry> entries_;
    std::vector<Collection> collections_;
    std::vector<std::string> watched_;
    bool dirty_ = false;

    friend bool loadIndex(const std::string& file, FileIndex& into);
};

bool loadIndex(const std::string& file, FileIndex& into);
bool saveIndex(const std::string& file, FileIndex& index);

}
