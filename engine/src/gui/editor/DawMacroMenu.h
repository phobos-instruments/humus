// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamUnit.h"
#include "gui/common/Localisation.h"
#include "gui/editor/RangePrompt.h"
#include "gui/host/BrickHost.h"

namespace hum {

inline constexpr int kDawMacroFirstId = 60, kDawMacroUnmapId = 59, kDawMacroRangeId = 58;

inline juce::String dawMacroTargetName(const DawMacroTarget& t) {
    return juce::String::fromUTF8(t.organism.c_str()) + " " + juce::String::fromUTF8(t.param.c_str());
}

inline juce::String dawMacroLabel(BrickHost& host, int slot, const std::string& organism, const std::string& param) {
    std::vector<DawMacroTarget> others;
    for (const auto& t : host.dawControlTargets(slot))
        if (!t.is(organism, param)) others.push_back(t);
    const auto m = juce::String(slot + 1);
    if (others.empty()) return tr("plugin-face.macro", "Macro M{n}", {{"n", m}});
    const auto first = dawMacroTargetName(others.front());
    if (others.size() == 1)
        return tr("plugin-face.macro-also", "Macro M{n}, also driving {target}", {{"n", m}, {"target", first}});
    return trn("plugin-face.macro-also-more", "Macro M{m}, also driving {target} and {n} more",
               "Macro M{m}, also driving {target} and {n} more", (long long) others.size() - 1,
               {{"m", m}, {"target", first}});
}

inline int addDawMacroMenu(juce::PopupMenu& menu, BrickHost& host, const std::string& organism,
                           const std::string& param) {
    const int count = host.dawControlCount();
    if (count <= 0) return -1;
    const int current = host.dawControlFor(organism, param);
    juce::PopupMenu macros;
    for (int i = 0; i < count; ++i)
        macros.addItem(kDawMacroFirstId + i, dawMacroLabel(host, i, organism, param), true, i == current);
    menu.addSubMenu(tr("plugin-face.map-to-macro", "Automate from the DAW"), macros);
    if (current >= 0) {
        const auto n = juce::String(current + 1);
        menu.addItem(kDawMacroRangeId, tr("plugin-face.macro-range", "Set the range Macro M{n} sweeps...", {{"n", n}}));
        menu.addItem(kDawMacroUnmapId, tr("plugin-face.unmap", "Stop automating from Macro M{n}", {{"n", n}}));
    }
    menu.addSeparator();
    return current;
}

inline void promptDawMacroRange(BrickHost& host, const std::string& organism, const std::string& param, int slot,
                                juce::Point<int> screen, double lo, double hi, Unit unit) {
    if (hi <= lo) return;
    DawMacroTarget mine{organism, param};
    for (const auto& t : host.dawControlTargets(slot))
        if (t.is(organism, param)) mine = t;
    const auto title = tr("plugin-face.macro-range-title", "Macro M{n} sweeps {target}",
                          {{"n", juce::String(slot + 1)}, {"target", dawMacroTargetName(mine)}});
    juce::MessageManager::callAsync([&host, organism, param, screen, title, lo, hi, unit, mine] {
        RangePrompt::show({screen.x, screen.y, 1, 1}, title, unit, lo + mine.from * (hi - lo),
                          lo + mine.to * (hi - lo), [&host, organism, param, lo, hi](double a, double b) {
                              host.setDawControlRange(organism, param, (a - lo) / (hi - lo), (b - lo) / (hi - lo));
                          });
    });
}

inline bool pickDawMacro(BrickHost& host, const std::string& organism, const std::string& param, int current, int r,
                         juce::Point<int> screen, double lo, double hi, Unit unit) {
    if (r == kDawMacroUnmapId && current >= 0) {
        host.removeDawControlTarget(organism, param);
        return true;
    }
    if (r == kDawMacroRangeId && current >= 0) {
        promptDawMacroRange(host, organism, param, current, screen, lo, hi, unit);
        return true;
    }
    if (r < kDawMacroFirstId || r >= kDawMacroFirstId + host.dawControlCount()) return false;
    const int slot = r - kDawMacroFirstId;
    if (slot == current) host.removeDawControlTarget(organism, param);
    else host.addDawControlTarget(slot, organism, param);
    return true;
}

}
