// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/BrowserPlaces.h"

#include <algorithm>

#include "core/browser/FileTimes.h"
#include "core/browser/FolderScan.h"
#include "core/packs/CatalogueKinds.h"

#include <cstdint>
#include <system_error>

#include "hum/FileBytes.h"

namespace hum::browser {

namespace {

std::string join(const std::string& dir, const std::string& leaf) {
    if (dir.empty()) return {};
    if (dir.back() == '/' || dir.back() == '\\') return dir + leaf;
    return dir + (dir.find('\\') != std::string::npos ? '\\' : '/') + leaf;
}

std::vector<const Entry*> under(const FileIndex& index, const std::string& dir) {
    std::vector<const Entry*> out;
    if (dir.empty()) return out;
    for (const auto& [path, e] : index.entries())
        if (isUnder(path, dir)) out.push_back(&e);
    return out;
}

Place placeOf(Place::Type type, std::string path = {}, std::string name = {}) {
    Place p;
    p.type = type;
    p.path = std::move(path);
    p.name = std::move(name);
    return p;
}

}

const std::vector<LibraryShelf>& libraryShelves() {
    static const std::vector<LibraryShelf> shelves = [] {
        std::vector<LibraryShelf> out;
        for (const auto& k : catalogue::kinds()) {
            LibraryShelf shelf{k.id.c_str(), {}};
            size_t from = 0;
            while (from < k.wildcard.size()) {
                const auto to = std::min(k.wildcard.find(';', from), k.wildcard.size());
                const auto pattern = k.wildcard.substr(from, to - from);
                const auto kind = kindOfFile("/" + k.id + "/x" + pattern.substr(pattern.find('.')));
                if (kind != Kind::Other && std::find(shelf.holds.begin(), shelf.holds.end(), kind) == shelf.holds.end())
                    shelf.holds.push_back(kind);
                from = to + 1;
            }
            if (k.id == "Impulses" && std::find(shelf.holds.begin(), shelf.holds.end(), Kind::Sound) == shelf.holds.end())
                shelf.holds.push_back(Kind::Sound);
            out.push_back(std::move(shelf));
        }
        return out;
    }();
    return shelves;
}

const char* shelfFor(Kind kind) {
    switch (kind) {
        case Kind::Sound: return "Samples";
        case Kind::Impulse: return "Impulses";
        case Kind::Bank: return "Banks";
        case Kind::Midi: return "Scores";
        case Kind::Scale: return "Scales";
        case Kind::Shader: return "Shaders";
        default: return nullptr;
    }
}

std::vector<Kind> kindsHeldBy(const Place& place) {
    switch (place.type) {
        case Place::Type::Projects: return {Kind::Project, Kind::Patch};
        case Place::Type::Recordings: return {Kind::Sound, Kind::Video};
        case Place::Type::Presets: return {Kind::Preset};
        case Place::Type::Shelf:
            for (const auto& s : libraryShelves())
                if (place.name == s.folder) return s.holds;
            return {};
        default: return {};
    }
}

bool placeHolds(const Place& place, const std::vector<Kind>& wanted) {
    if (wanted.empty()) return true;
    const auto held = kindsHeldBy(place);
    if (held.empty()) return true;
    return std::any_of(wanted.begin(), wanted.end(),
                       [&](Kind k) { return std::find(held.begin(), held.end(), k) != held.end(); });
}

std::vector<SidebarItem> sidebarFor(const FileIndex& index, const PlaceRoots& roots, const std::vector<Kind>& wanted) {
    std::vector<SidebarItem> out;
    auto add = [&](SidebarItem::Section section, Place place, std::string label) {
        SidebarItem item;
        item.section = section;
        item.dimmed = !placeHolds(place, wanted);
        item.place = std::move(place);
        item.label = std::move(label);
        out.push_back(std::move(item));
    };
    using S = SidebarItem::Section;
    add(S::Top, placeOf(Place::Type::Recent), "Recent");
    add(S::Top, placeOf(Place::Type::Favourites), "Favourites");
    add(S::Top, placeOf(Place::Type::Projects), "Projects");
    for (const auto& s : libraryShelves())
        add(S::Library, placeOf(Place::Type::Shelf, join(roots.library, s.folder), s.folder), s.folder);
    add(S::Yours, placeOf(Place::Type::Recordings, roots.recordings), "Recordings");
    add(S::Yours, placeOf(Place::Type::Presets, roots.presets), "Presets");
    for (const auto& f : roots.computer) add(S::Computer, placeOf(Place::Type::Folder, f.path, f.label), f.label);
    for (const auto& w : index.watched()) add(S::Watched, placeOf(Place::Type::Watched, w), fileName(w));
    for (const auto& c : index.collections()) add(S::Collections, placeOf(Place::Type::Collection, {}, c.name), c.name);
    return out;
}

std::vector<const Entry*> entriesIn(const FileIndex& index, const Place& place, const PlaceRoots& roots) {
    switch (place.type) {
        case Place::Type::Recent: return index.recent();
        case Place::Type::Favourites: return index.favourites();
        case Place::Type::Projects: {
            std::vector<const Entry*> out;
            for (const auto& [path, e] : index.entries()) {
                if (e.kind == Kind::Project) { out.push_back(&e); continue; }
                if (e.kind != Kind::Patch) continue;
                const auto* home = index.find(parentOf(path));
                if (home == nullptr || home->kind != Kind::Project) out.push_back(&e);
            }
            return out;
        }
        case Place::Type::Collection: {
            std::vector<const Entry*> out;
            if (const auto* c = index.collection(place.name))
                for (const auto& path : c->paths)
                    if (const auto* e = index.find(path)) out.push_back(e);
            return out;
        }
        default: return under(index, place.path);
    }
}

Place folderPlace(const std::string& dir) { return placeOf(Place::Type::Folder, dir, fileName(dir)); }

std::vector<Entry> listFolder(const std::string& dir, const FileIndex& index) {
    std::vector<Entry> out;
    std::error_code ec;
    std::filesystem::directory_iterator it(utf8Path(dir), std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) return out;
    for (const auto& item : it) {
        const auto name = utf8Text(item.path().filename());
        if (name.empty() || name[0] == '.') continue;
        Entry e;
        e.path = utf8Text(item.path());
        if (item.is_directory(ec)) {
            e.kind = kindOfFolder(e.path);
            if (e.kind == Kind::Other) e.kind = Kind::Folder;
        } else if (item.is_regular_file(ec)) {
            e.kind = kindOfFile(e.path);
            if (e.kind == Kind::Other) continue;
            e.size = (std::int64_t) item.file_size(ec);
        } else {
            continue;
        }
        e.modified = secondsSinceEpoch(item.last_write_time(ec));
        e.created = createdSeconds(e.path);
        if (const auto* known = index.find(e.path)) {
            const auto kind = e.kind;
            e = *known;
            e.kind = kind;
        }
        out.push_back(std::move(e));
    }
    return out;
}

std::vector<NamedFolder> crumbsFor(const std::string& dir, const PlaceRoots& roots) {
    const NamedFolder* base = nullptr;
    for (const auto& f : roots.computer)
        if ((dir == f.path || isUnder(dir, f.path)) && (base == nullptr || f.path.size() > base->path.size())) base = &f;
    std::vector<NamedFolder> out;
    if (base != nullptr) out.push_back(*base);
    size_t from = base != nullptr ? std::min(dir.size(), base->path.size()) : 0;
    while (from < dir.size()) {
        const auto slash = dir.find_first_of("/\\", from);
        const auto to = slash == std::string::npos ? dir.size() : slash;
        if (to > from) out.push_back({dir.substr(from, to - from), dir.substr(0, to)});
        from = to + 1;
    }
    return out;
}

std::string whereText(const std::string& path, const Place& place, const PlaceRoots& roots) {
    const auto parent = parentOf(path);
    if (!place.path.empty() && isUnder(parent, place.path)) return parent.substr(place.path.size() + 1);
    if (!place.path.empty() && parent == place.path) return {};
    if (!roots.home.empty() && isUnder(parent, roots.home)) return "~" + parent.substr(roots.home.size());
    return parent;
}

Place homeFor(const std::vector<Kind>& wanted, const std::string& current, const PlaceRoots& roots, const FileIndex& index) {
    if (std::find(wanted.begin(), wanted.end(), Kind::Project) != wanted.end()) return placeOf(Place::Type::Recent);
    for (const auto& item : sidebarFor(index, roots, wanted)) {
        if (item.place.path.empty() || !isUnder(current, item.place.path) || item.dimmed) continue;
        return item.place.type == Place::Type::Folder ? folderPlace(parentOf(current)) : item.place;
    }
    if (!wanted.empty())
        for (const auto& s : libraryShelves())
            if (s.holds.front() == wanted.front()) return placeOf(Place::Type::Shelf, join(roots.library, s.folder), s.folder);
    return placeOf(Place::Type::Recent);
}

}
