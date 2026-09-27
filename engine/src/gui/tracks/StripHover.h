// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/common/Localisation.h"
#include "gui/tracks/StripSources.h"

namespace hum {

inline juce::String stripSourceTip(const std::string& param,
                                   const std::vector<ConnectionModel>& cords,
                                   const std::string& node) {
    if (stripChannelInlets(param).empty()) return {};
    const auto names = stripSourceNames(param, cords, node);
    if (names.empty()) return tr("strip-hover.nothing-connected", "Nothing connected");
    return juce::String::fromUTF8(joinedSources(names).c_str());
}

}
