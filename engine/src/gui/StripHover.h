#pragma once
#include <cstdlib>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "io/PatchDocument.h"
#include "gui/Localisation.h"

namespace hum {

inline std::vector<int> stripChannelInlets(const std::string& param) {
    const auto us = param.rfind('_');
    if (us == std::string::npos || us + 1 >= param.size()) return {};
    const auto sfx = param.substr(us + 1);
    int lo = 0, hi = 0;
    const auto dash = sfx.find('-');
    for (char ch : sfx)
        if ((ch < '0' || ch > '9') && ch != '-') return {};
    if (dash == std::string::npos) {
        lo = hi = std::atoi(sfx.c_str());
    } else {
        lo = std::atoi(sfx.substr(0, dash).c_str());
        hi = std::atoi(sfx.substr(dash + 1).c_str());
    }
    if (lo < 1 || hi < lo || hi - lo > 15) return {};
    std::vector<int> out;
    for (int c = lo; c <= hi; ++c) out.push_back(c - 1);
    return out;
}

inline juce::String stripSourceTip(const std::string& param,
                                   const std::vector<ConnectionModel>& cords,
                                   const std::string& node) {
    const auto inlets = stripChannelInlets(param);
    if (inlets.empty()) return {};
    juce::StringArray names;
    for (const auto& c : cords) {
        if (c.dst != node) continue;
        for (int in : inlets)
            if (c.dstInlet == in) {
                names.addIfNotAlreadyThere(juce::String::fromUTF8(c.src.c_str()));
                break;
            }
    }
    if (names.isEmpty()) return tr("strip-hover.nothing-connected", "Nothing connected");
    return names.joinIntoString(" + ");
}

}
