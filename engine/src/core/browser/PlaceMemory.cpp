// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/PlaceMemory.h"

#include <filesystem>
#include <system_error>

#include "hum/FileBytes.h"

namespace hum::browser {

namespace {

constexpr char kSep = '\n';
constexpr int kLastType = (int) Place::Type::Folder;

bool isFolder(const std::string& path) {
    std::error_code ec;
    return !path.empty() && std::filesystem::is_directory(utf8Path(path), ec);
}

}

std::string encodePlace(const Place& place) {
    return std::to_string((int) place.type) + kSep + place.path + kSep + place.name;
}

std::optional<Place> decodePlace(const std::string& text) {
    const auto a = text.find(kSep);
    const auto b = a == std::string::npos ? a : text.find(kSep, a + 1);
    if (b == std::string::npos) return std::nullopt;
    const auto head = text.substr(0, a);
    if (head.empty() || head.find_first_not_of("0123456789") != std::string::npos || head.size() > 2) return std::nullopt;
    const int type = std::stoi(head);
    if (type > kLastType) return std::nullopt;
    Place p;
    p.type = (Place::Type) type;
    p.path = text.substr(a + 1, b - a - 1);
    p.name = text.substr(b + 1);
    return p;
}

bool placeStillThere(const Place& place, const std::vector<Kind>& wanted, const PlaceRoots& roots, const FileIndex& index) {
    if (!placeHolds(place, wanted)) return false;
    if (place.type == Place::Type::Folder) return isFolder(place.path);
    if (!place.path.empty() && !isFolder(place.path)) return false;
    for (const auto& item : sidebarFor(index, roots, wanted))
        if (item.place == place) return !item.dimmed;
    return false;
}

}
