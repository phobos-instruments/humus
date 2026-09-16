// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/plugins/PluginHost.h"

namespace hum {

inline std::vector<std::pair<std::string, CategoryGroups>> classPickerGroups() {
    return groupByOrigin(PluginHost::allPaletteClasses());
}

inline juce::PopupMenu classPickerMenu(const std::vector<std::pair<std::string, CategoryGroups>>& groups,
                                       int base, std::vector<std::string>& order) {
    juce::PopupMenu m;
    int i = 0;
    for (auto& o : groups) {
        std::vector<std::vector<std::string>> segs;
        segs.reserve(o.second.size());
        for (auto& g : o.second) segs.push_back(categorySegments(g.first));

        std::function<juce::PopupMenu(size_t, size_t, size_t)> build =
            [&](size_t lo, size_t hi, size_t depth) {
                juce::PopupMenu menu;
                size_t k = lo;
                while (k < hi) {
                    if (depth >= segs[k].size()) {
                        for (auto& cls : o.second[k].second) {
                            order.push_back(cls);
                            menu.addItem(base + (++i),
                                         juce::String::fromUTF8(parseClassString(cls).display.c_str()));
                        }
                        ++k;
                        continue;
                    }
                    const std::string& s = segs[k][depth];
                    size_t e = k + 1;
                    while (e < hi && segs[e].size() > depth && segs[e][depth] == s) ++e;
                    menu.addSubMenu(juce::String::fromUTF8(s.c_str()), build(k, e, depth + 1));
                    k = e;
                }
                return menu;
            };
        m.addSubMenu(juce::String::fromUTF8(o.first.c_str()), build(0, o.second.size(), 0));
    }
    return m;
}

inline juce::PopupMenu classPickerMenu(int base, std::vector<std::string>& order) {
    return classPickerMenu(classPickerGroups(), base, order);
}

}
