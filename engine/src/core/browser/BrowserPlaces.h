// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/browser/FileIndex.h"

namespace hum::browser {

struct LibraryShelf {
    const char* folder;
    std::vector<Kind> holds;
};

const std::vector<LibraryShelf>& libraryShelves();
const char* shelfFor(Kind kind);

struct NamedFolder {
    std::string label;
    std::string path;
};

struct PlaceRoots {
    std::string library;
    std::string patches;
    std::string recordings;
    std::string presets;
    std::vector<NamedFolder> computer;
    std::string home;
};

struct Place {
    enum class Type { Recent, Favourites, Projects, Shelf, Recordings, Presets, Watched, Collection, Folder };
    Type type = Type::Recent;
    std::string path;
    std::string name;

    bool operator==(const Place& o) const { return type == o.type && path == o.path && name == o.name; }
    bool operator!=(const Place& o) const { return !(*this == o); }
};

struct SidebarItem {
    enum class Section { Top, Library, Yours, Computer, Watched, Collections };
    Section section = Section::Top;
    Place place;
    std::string label;
    bool dimmed = false;
    double progress = -1.0;
};

std::vector<Kind> kindsHeldBy(const Place& place);
bool placeHolds(const Place& place, const std::vector<Kind>& wanted);
std::vector<SidebarItem> sidebarFor(const FileIndex& index, const PlaceRoots& roots, const std::vector<Kind>& wanted);
std::vector<const Entry*> entriesIn(const FileIndex& index, const Place& place, const PlaceRoots& roots);
std::vector<Entry> listFolder(const std::string& dir, const FileIndex& index);
std::string whereText(const std::string& path, const Place& place, const PlaceRoots& roots);
Place folderPlace(const std::string& dir);
std::vector<NamedFolder> crumbsFor(const std::string& dir, const PlaceRoots& roots);
Place homeFor(const std::vector<Kind>& wanted, const std::string& current, const PlaceRoots& roots, const FileIndex& index);

}
