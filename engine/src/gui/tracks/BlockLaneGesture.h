// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "gui/editor/BankSlotSpec.h"
#include "io/PatchDocument.h"

namespace hum {

struct BlockGesture {
    enum class Mode { Paint, Edge, Move, Copy };
    Mode mode = Mode::Paint;
    int slot = -1;
    std::string node, param;
    std::vector<AutomationBreakpoint> before;
    int edge = -1;
    int block = -1;
    int value = 0;
    int fallback = 0;
    double anchor = 0.0;
    int downX = 0;
    bool changed = false;
    bool groupWasSelected = false;
    bool inGap = false;
    double start = 0.0;
    bool active() const { return slot >= 0; }
};

inline const std::vector<std::string>& blockLabelsFor(const std::string& cls, const std::string& param) {
    static std::map<std::pair<std::string, std::string>, std::vector<std::string>> cache;
    const auto key = std::make_pair(cls, param);
    if (const auto it = cache.find(key); it != cache.end()) return it->second;
    std::vector<std::string> labels;
    for (const auto& c : layoutSpecFor(cls).controls)
        if (c.param == param && !c.options.empty()) {
            labels = c.options;
            break;
        }
    return cache.emplace(key, std::move(labels)).first->second;
}

inline int blockChoiceCount(const std::vector<std::string>& labels, double lo, double hi) {
    if (!labels.empty()) return (int) labels.size();
    return hi > lo ? (int) (hi - lo) + 1 : 1;
}

}
