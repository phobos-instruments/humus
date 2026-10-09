// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_core/juce_core.h>

#include "core/packs/Categories.h"
#include "gui/common/Localisation.h"

namespace hum {

inline juce::String categoryText(const std::string& category) {
    juce::StringArray out;
    for (const auto& segment : categorySegments(category)) {
        out.add(trKey("category." + segment, juce::String::fromUTF8(segment.c_str())));
    }
    return out.joinIntoString(juce::String::fromUTF8(" \xc2\xb7 "));
}

inline juce::String pickerTagText(const std::string& tag) {
    const auto separator = juce::String::fromUTF8(" \xe2\x80\xba ");
    const juce::String text(tag);
    if (!text.contains(separator)) return categoryText(tag);
    return text.upToFirstOccurrenceOf(separator, false, false) + separator
         + categoryText(text.fromFirstOccurrenceOf(separator, false, false).toStdString());
}

inline juce::String familyText(Family f) {
    switch (f) {
        case Family::Voice: return tr("help-family.voice", "Voice family");
        case Family::Time: return tr("help-family.time", "Time family");
        case Family::Motion: return tr("help-family.motion", "Motion family");
        case Family::Sense: return tr("help-family.sense", "Sense family");
        case Family::Utility: break;
    }
    return tr("help-family.utility", "Utility family");
}

}
