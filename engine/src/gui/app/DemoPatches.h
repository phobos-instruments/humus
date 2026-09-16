// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "gui/common/Localisation.h"

namespace hum {

inline juce::File demoPatchesDir() {
    const auto path = assetSearchPath("patches");
    return path.empty() ? juce::File() : path.front();
}

inline juce::Array<juce::File> demoPatches() {
    juce::Array<juce::File> out;
    juce::StringArray seen;
    for (const auto& dir : assetSearchPath("patches"))
        for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.hum;*.amh")) {
            if (f.getFileName().startsWith("my-") || seen.contains(f.getFileName())) continue;
            seen.add(f.getFileName());
            out.add(f);
        }
    struct { int compareElements(const juce::File& a, const juce::File& b) {
        return a.getFileName().compareIgnoreCase(b.getFileName()); } } byName;
    out.sort(byName);
    return out;
}

inline juce::String demoPatchTitle(const juce::File& patch) {
    auto name = patch.getFileNameWithoutExtension().replaceCharacters("-_", "  ").trim();
    return name.isEmpty() ? name : name.substring(0, 1).toUpperCase() + name.substring(1);
}

inline juce::String demoPatchBlurb(const juce::File& patch) {
    const auto note = patch.getParentDirectory()
                          .getChildFile(patch.getFileNameWithoutExtension() + ".txt");
    if (note.existsAsFile()) {
        const auto line = note.loadFileAsString().upToFirstOccurrenceOf("\n", false, false).trim();
        if (line.isNotEmpty()) return line;
    }
    return tr("start.a-patch-that-ships-with", "a patch that ships with Humus");
}

}
