// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include <hum/Registry.h>

#include "core/packs/PackRegistry.h"
#include "gui/help/HelpDocs.h"

namespace hum {

namespace help_detail {

inline juce::String tidy(const juce::String& s) {
    juce::String t = s;
    t = t.replace("\t", " ");
    while (t.contains("  ")) t = t.replace("  ", " ");
    juce::String tight;
    tight.preallocateBytes((size_t) t.getNumBytesAsUTF8());
    for (int i = 0; i < t.length(); ++i) {
        const bool spaceBeforeMark = t[i] == ' ' && i + 1 < t.length()
                                  && juce::String(".,;:!?").containsChar(t[i + 1]);
        const bool markOpensAWord = spaceBeforeMark && i + 2 < t.length()
                                 && juce::CharacterFunctions::isLetterOrDigit(t[i + 2]);
        if (spaceBeforeMark && !markOpensAWord) continue;
        tight += t[i];
    }
    t = tight;
    t = t.replace("( ", "(").replace(" )", ")");
    return t.trim();
}

struct Block {
    enum Kind { Header, Para, Def, Image } kind;
    juce::String a, b;
    int level = 0;
    bool rich = false;
    bool bullet = false;
};

struct LinkToken { juce::String text; bool link = false; };
inline std::vector<LinkToken> linkTokens(const juce::String& s) {
    std::vector<LinkToken> out;
    auto add = [&](const juce::String& t, bool link) {
        if (t.isEmpty()) return;
        if (!link && !out.empty() && !out.back().link) { out.back().text += t; return; }
        out.push_back({t, link});
    };
    juce::String word;
    for (int i = 0; i <= s.length(); ++i) {
        const juce::juce_wchar c = i < s.length() ? s[i] : 0;
        if (c != 0 && juce::CharacterFunctions::isLetterOrDigit(c)) { word += c; continue; }
        if (word.isNotEmpty()) {
            const auto w = word.toStdString();
            add(word, PackRegistry::instance().classManifest(w) != nullptr
                          || Registry::instance().isKnown(w));
            word.clear();
        }
        if (c != 0) add(juce::String::charToString(c), false);
    }
    return out;
}

inline bool isRelatedHeader(const juce::String& t) {
    const auto low = t.trim().toLowerCase();
    return low == "related organisms" || low == "see also";
}

}

}
