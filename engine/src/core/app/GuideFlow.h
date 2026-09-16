// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>

#include <BinaryData.h>
#include <juce_core/juce_core.h>

namespace hum {
namespace guide {

enum Page { kWelcome = 0, kPatch, kFold, kRemembers, kRoot, kNumPages };

inline Page next(Page p) { return p >= kRoot ? kRoot : (Page) (p + 1); }
inline Page back(Page p) { return p <= kWelcome ? kWelcome : (Page) (p - 1); }
inline bool isFirst(Page p) { return p == kWelcome; }
inline bool isLast(Page p) { return p == kRoot; }

inline const char* pageFile(Page p) {
    switch (p) {
        case kWelcome:   return "1-welcome.md";
        case kPatch:     return "2-plant-and-cord.md";
        case kFold:      return "3-pods-and-packs.md";
        case kRemembers: return "4-the-soil-remembers.md";
        case kRoot:      return "5-take-root.md";
        default:         return "";
    }
}

namespace detail {

struct Loaded { std::string title, body; };

inline const Loaded& page(Page p) {
    static std::map<int, Loaded> cache;
    if (const auto it = cache.find((int) p); it != cache.end()) return it->second;

    Loaded out;
    const juce::String want(pageFile(p));
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        const auto* res = BinaryData::namedResourceList[i];
        if (want != BinaryData::getNamedResourceOriginalFilename(res)) continue;
        int size = 0;
        const char* data = BinaryData::getNamedResource(res, size);
        if (data == nullptr || size <= 0) break;
        const auto raw = juce::String::fromUTF8(data, size).replace("\r\n", "\n");
        out.title = raw.upToFirstOccurrenceOf("\n", false, false)
                       .trimCharactersAtStart("# ").trim().toStdString();
        out.body = raw.fromFirstOccurrenceOf("\n", false, false)
                      .trimCharactersAtStart("\n").toStdString();
        break;
    }
    return cache.emplace((int) p, std::move(out)).first->second;
}

}

inline const std::string& pageTitle(Page p) { return detail::page(p).title; }
inline const std::string& pageBody(Page p) { return detail::page(p).body; }

}
}
