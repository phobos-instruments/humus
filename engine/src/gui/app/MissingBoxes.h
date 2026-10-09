// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/host/EngineHost.h"
#include "gui/common/Localisation.h"

namespace hum::missingboxes {

inline constexpr int kNamedInLine = 2;

inline std::vector<std::string> unavailableNames(EngineHost& host) {
    std::vector<std::string> names;
    for (const auto& cm : host.model().organisms)
        if (!host.missingClassNote(cm.name).empty()) names.push_back(cm.name);
    return names;
}

inline juce::String boxName(const std::string& name) {
    return juce::String(name);
}

inline juce::String statusLine(const std::vector<std::string>& names) {
    const int count = (int) names.size();
    if (count == 0) return {};
    const int named = count > kNamedInLine ? kNamedInLine : count;
    juce::String s = boxName(names.front());
    for (int i = 1; i < named; ++i)
        s = i == count - 1 ? tr("missing-boxes.and", "{list} and {name}", {{"list", s}, {"name", boxName(names[(size_t) i])}})
                           : s + ", " + boxName(names[(size_t) i]);
    if (count > named)
        s = trn("missing-boxes.more", "{list} and {n} more", "{list} and {n} more", count - named, {{"list", s}});
    return trn("missing-boxes.unavailable", "{names} is unavailable here and passes sound straight through",
               "{names} are unavailable here and pass sound straight through", count, {{"names", s}});
}

}
