// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>
#include <utility>

#include <BinaryData.h>
#include <juce_core/juce_core.h>

#include "core/app/PageLanguage.h"

namespace hum {

struct EmbeddedPage {
    std::string file, text;
};

inline EmbeddedPage embeddedPage(const std::string& stem) {
    for (const auto& file : embeddedCandidates(stem))
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
            const auto* res = BinaryData::namedResourceList[i];
            if (juce::String(file) != BinaryData::getNamedResourceOriginalFilename(res)) continue;
            int size = 0;
            const char* data = BinaryData::getNamedResource(res, size);
            if (data != nullptr && size > 0) return {file, withoutFrontMatter(std::string(data, (size_t) size))};
        }
    return {};
}

namespace guide {

enum Page { kWelcome = 0, kPatch, kFold, kRemembers, kRoot, kNumPages };

inline Page next(Page p) { return p >= kRoot ? kRoot : (Page) (p + 1); }
inline Page back(Page p) { return p <= kWelcome ? kWelcome : (Page) (p - 1); }
inline bool isFirst(Page p) { return p == kWelcome; }
inline bool isLast(Page p) { return p == kRoot; }

inline const char* pageStem(Page p) {
    switch (p) {
        case kWelcome:   return "1-welcome";
        case kPatch:     return "2-plant-and-cord";
        case kFold:      return "3-pods-and-packs";
        case kRemembers: return "4-the-soil-remembers";
        case kRoot:      return "5-take-root";
        default:         return "";
    }
}

namespace detail {

struct Loaded { std::string title, body; };

inline const Loaded& page(Page p) {
    static std::map<std::pair<int, std::string>, Loaded> cache;
    const auto key = std::make_pair((int) p, pageLanguage());
    if (const auto it = cache.find(key); it != cache.end()) return it->second;

    Loaded out;
    const auto found = embeddedPage(pageStem(p));
    if (!found.text.empty()) {
        const auto raw = juce::String::fromUTF8(found.text.data(), (int) found.text.size()).replace("\r\n", "\n");
        out.title = raw.upToFirstOccurrenceOf("\n", false, false)
                       .trimCharactersAtStart("# ").trim().toStdString();
        out.body = raw.fromFirstOccurrenceOf("\n", false, false)
                      .trimCharactersAtStart("\n").toStdString();
    }
    return cache.emplace(key, std::move(out)).first->second;
}

}

inline const std::string& pageTitle(Page p) { return detail::page(p).title; }
inline const std::string& pageBody(Page p) { return detail::page(p).body; }

}
}
