// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_core/juce_core.h>

namespace hum {

inline constexpr const char* kPatchExt = "hum";
inline constexpr const char* kLegacyPatchExt = "amh";
inline constexpr const char* kPatchOpenFilter = "*.hum;*.amh";

inline juce::String toDocumentPath(const juce::String& nativeRelative) {
    return nativeRelative.replaceCharacter('\\', '/');
}

inline juce::String fromDocumentPath(const juce::String& stored) {
    return stored.replaceCharacter('\\', '/');
}

}
