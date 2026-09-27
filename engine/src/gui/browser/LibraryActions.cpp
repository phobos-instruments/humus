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

int removeFromLibrary(const std::vector<std::string>& paths, const std::string& libraryRoot, BrowserIndex& index,
                      const Discard& discard) {
    int removed = 0;
    for (const auto& path : paths) {
        if (!isUnder(path, libraryRoot)) continue;
        const auto f = fileAt(path);
        if (f.exists() && !discard(f)) continue;
        index.edit([&](FileIndex& i) { i.forget(path); });
        ++removed;
    }
    return removed;
}

}
