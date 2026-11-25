// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatternText.h"

#include "core/xml/Xml.h"
#include "io/PatchParseInternal.h"
#include "io/PatchWriteInternal.h"

namespace hum {

std::string patternToText(const Pattern& pattern) {
    if (!pattern.present) return {};
    xml::Element holder("holder");
    writePattern(holder, pattern);
    xml::Format format;
    format.header = false;
    return xml::write(*holder.child("pattern"), format.singleLine());
}

bool patternFromText(const std::string& text, Pattern& pattern) {
    if (text.empty()) return false;
    auto root = xml::parse(text);
    if (!root || !root->hasTag("pattern")) return false;
    Pattern parsed;
    parsePattern(*root, parsed);
    pattern = std::move(parsed);
    return true;
}

}
