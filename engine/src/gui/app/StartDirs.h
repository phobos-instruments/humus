// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "gui/app/AppSettings.h"

namespace hum {

enum class DirPurpose { Patch, Recording, Preset };

inline const char* dirPurposeKey(DirPurpose p) {
    switch (p) {
        case DirPurpose::Recording: return "lastDir.recording";
        case DirPurpose::Preset:    return "lastDir.preset";
        case DirPurpose::Patch:     break;
    }
    return "lastDir.save";
}

inline juce::File dirPurposeHome(DirPurpose p) {
    switch (p) {
        case DirPurpose::Recording: return userRecordingsDir();
        case DirPurpose::Preset:    return userPresetsDir();
        case DirPurpose::Patch:     break;
    }
    return userPatchesDir();
}

inline juce::File startDirFor(DirPurpose p) {
    const juce::File kept(AppSettings::instance().getString(dirPurposeKey(p), ""));
    if (kept.isDirectory()) return kept;
    const auto home = dirPurposeHome(p);
    home.createDirectory();
    return home.isDirectory() ? home
                              : juce::File::getSpecialLocation(juce::File::userHomeDirectory);
}

inline void rememberDirFor(DirPurpose p, const juce::File& chosen) {
    const auto dir = chosen.isDirectory() ? chosen : chosen.getParentDirectory();
    if (!dir.isDirectory()) return;
    AppSettings::instance().set(dirPurposeKey(p), dir.getFullPathName());
}

}
