// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace hum {

void PatcherCanvas::toggleCord(const Edge& e) {
    if (const auto it = std::find(selectedCords_.begin(), selectedCords_.end(), e); it != selectedCords_.end())
        selectedCords_.erase(it);
    else
        selectedCords_.push_back(e);
}

void PatcherCanvas::forgetCords(const std::vector<Edge>& cords) {
    selectedCords_.erase(std::remove_if(selectedCords_.begin(), selectedCords_.end(),
                                        [&cords](const Edge& e) {
                                            return std::find(cords.begin(), cords.end(), e) != cords.end();
                                        }),
                         selectedCords_.end());
}

std::vector<PatcherCanvas::Edge> PatcherCanvas::liveSelectedCords() {
    std::vector<Edge> out;
    for (const auto& line : visibleCords())
        if (isCordSelected(line.cord)) out.push_back(line.cord);
    return out;
}

void PatcherCanvas::removeCords(const std::vector<Edge>& cords) {
    if (cords.empty()) return;
    host_.beginTransaction();
    for (const auto& c : cords) removeCord(c);
    host_.endTransaction();
    forgetCords(cords);
}

void PatcherCanvas::beginBand(juce::Point<int> p, bool cords, bool additive) {
    if (!additive) {
        selection_.clear();
        primary_.clear();
        selectedCords_.clear();
        notifySelection();
    }
    bandNodes_ = selection_;
    bandCords_ = selectedCords_;
    drag_ = cords ? Drag::CordBand : Drag::Select;
    dragStart_ = p;
    selectRect_ = {p.x, p.y, 0, 0};
    refresh();
}

void PatcherCanvas::dragBand(juce::Point<int> p) {
    selectRect_ = juce::Rectangle<int>(dragStart_, p);
    if (drag_ == Drag::CordBand) {
        selectedCords_ = bandCords_;
        for (const auto& c : cordsCrossing(selectRect_))
            if (!isCordSelected(c)) selectedCords_.push_back(c);
    } else {
        selection_ = bandNodes_;
        for (auto& d : displayNodes())
            if (selectRect_.intersects(nodeBounds(d.name))) selection_.insert(d.name);
    }
    refresh();
}

}
