// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/LibraryActions.h"

#include "core/app/AppPaths.h"
#include "core/browser/BrowserPlaces.h"
#include "core/browser/FolderScan.h"

namespace hum::browser {

namespace {

Kind kindOf(const juce::File& f) {
    return f.isDirectory() ? kindOfFolder(pathOf(f)) : kindOfFile(pathOf(f));
}

juce::File landingFor(const juce::File& source, const juce::File& shelf) {
    const auto wanted = shelf.getChildFile(source.getFileName());
    if (!wanted.exists()) return wanted;
    if (!source.isDirectory() && wanted.existsAsFile() && wanted.getSize() == source.getSize()) return wanted;
    return wanted.getNonexistentSibling(true);
}

}

bool importable(const std::string& path, const std::string& libraryRoot) {
    const auto f = fileAt(path);
    return f.exists() && !isUnder(path, libraryRoot) && shelfFor(kindOf(f)) != nullptr;
}

std::vector<std::string> importToLibrary(const std::vector<std::string>& paths, const std::string& libraryRoot,
                                         BrowserIndex& index) {
    std::vector<std::string> landed;
    for (const auto& path : paths) {
        if (!importable(path, libraryRoot)) continue;
        const auto source = fileAt(path);
        const auto shelf = fileAt(libraryRoot).getChildFile(shelfFor(kindOf(source)));
        if (!shelf.createDirectory()) continue;
        const auto target = landingFor(source, shelf);
        const bool ok = target.exists() || (source.isDirectory() ? source.copyDirectoryTo(target) : source.copyFileTo(target));
        if (!ok) continue;
        const auto to = pathOf(target);
        index.edit([&](FileIndex& i) {
            i.observe(to, target.isDirectory() ? 0 : target.getSize(), target.getLastModificationTime().toMilliseconds() / 1000,
                      index.now(), kindOf(target));
            i.carryAnnotations(path, to, index.now());
        });
        landed.push_back(to);
    }
    return landed;
}

Discard toTrash() {
    return [](const juce::File& f) { return f.moveToTrash(); };
}

bool canTrash(const std::string& path, const TrashGuard& guard) {
    const auto f = fileAt(path);
    if (!f.exists()) return false;
    const auto kind = kindOf(f);
    if (!isUnder(path, guard.libraryRoot) && kind != Kind::Project && kind != Kind::Patch) return false;
    if (f.isDirectory() && f.getParentDirectory() == fileAt(guard.libraryRoot)) return false;
    for (const auto& place : guard.places)
        if (path == place || isUnder(place, path)) return false;
    for (const auto& root : guard.shipped)
        if (path == root || isUnder(path, root)) return false;
    return guard.openDocument.empty() || (guard.openDocument != path && !isUnder(guard.openDocument, path));
}

std::vector<std::string> trashable(const std::vector<std::string>& paths, const TrashGuard& guard) {
    std::vector<std::string> out;
    for (const auto& path : paths)
        if (canTrash(path, guard)) out.push_back(path);
    return out;
}

std::vector<std::string> moveToTrash(const std::vector<std::string>& paths, const TrashGuard& guard, BrowserIndex& index,
                                     const Discard& discard) {
    std::vector<std::string> gone;
    for (const auto& path : trashable(paths, guard)) {
        if (!discard(fileAt(path))) continue;
        index.edit([&](FileIndex& i) { i.forget(path); });
        gone.push_back(path);
    }
    return gone;
}

}
