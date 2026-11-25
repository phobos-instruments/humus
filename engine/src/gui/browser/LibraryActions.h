// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/browser/BrowserIndex.h"

namespace hum::browser {

bool importable(const std::string& path, const std::string& libraryRoot);
std::vector<std::string> importToLibrary(const std::vector<std::string>& paths, const std::string& libraryRoot,
                                         BrowserIndex& index);

struct TrashGuard {
    std::string libraryRoot;
    std::vector<std::string> places;
    std::vector<std::string> shipped;
    std::string openDocument;
};

using Discard = std::function<bool(const juce::File&)>;
Discard toTrash();
bool canTrash(const std::string& path, const TrashGuard& guard);
std::vector<std::string> trashable(const std::vector<std::string>& paths, const TrashGuard& guard);
std::vector<std::string> moveToTrash(const std::vector<std::string>& paths, const TrashGuard& guard, BrowserIndex& index,
                                     const Discard& discard = toTrash());

}
