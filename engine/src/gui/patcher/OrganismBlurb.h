// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>

#include <juce_core/juce_core.h>

#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"
#include "core/app/PickerModel.h"
#include "gui/help/HelpDocs.h"
#include "gui/help/HelpMarkdown.h"

namespace hum {

inline constexpr int kBlurbMaxChars = 140;

inline juce::String organismBlurb(const std::string& cls) {
    static std::map<std::string, juce::String> cache;
    if (const auto it = cache.find(cls); it != cache.end()) return it->second;
    juce::String out;
    const auto display = parseClassString(cls).display;
    if (const auto* m = PackRegistry::instance().classManifest(display); m && !m->blurb.empty())
        out = juce::String::fromUTF8(m->blurb.c_str());
    if (out.isEmpty()) {
        const auto doc = loadOrganismDoc(display);
        if (doc.text.isNotEmpty()) {
            const auto blocks = help_detail::parseHelpDoc(doc, juce::String(display)).second;
            for (const auto& b : blocks)
                if (b.kind == help_detail::Block::Para && b.a.isNotEmpty()) {
                    out = juce::String::fromUTF8(picker::firstSentence(b.a.toStdString()).c_str());
                    break;
                }
        }
    }
    if (out.length() > kBlurbMaxChars)
        out = out.substring(0, kBlurbMaxChars).upToLastOccurrenceOf(" ", false, false)
            + juce::String::fromUTF8("\xe2\x80\xa6");
    cache[cls] = out;
    return out;
}

}
