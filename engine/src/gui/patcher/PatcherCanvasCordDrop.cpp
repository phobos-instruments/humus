// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <string>
#include <utility>
#include <vector>

#include "gui/patcher/PatcherCanvas.h"

namespace hum {

std::vector<std::pair<int, int>> PatcherCanvas::cordsForDrop(const std::string& node, int port,
                                                             const juce::ModifierKeys& mods) const {
    int sIns = 0, sOuts = 0, dIns = 0, dOuts = 0;
    portCounts(dragNode_, sIns, sOuts);
    portCounts(node, dIns, dOuts);
    std::vector<std::pair<int, int>> cords{{cordOutlet_, port}};
    if (mods.isShiftDown() && mods.isAltDown()) {
        for (int in = port + 1; in < dIns; ++in) cords.push_back({cordOutlet_, in});
    } else if (mods.isShiftDown()) {
        for (int k = 1; cordOutlet_ + k < sOuts && port + k < dIns; ++k)
            cords.push_back({cordOutlet_ + k, port + k});
    } else if (!mods.isAltDown() && cordOutlet_ % 2 == 0 && port % 2 == 0
               && cordOutlet_ + 1 < sOuts && port + 1 < dIns) {
        cords.push_back({cordOutlet_ + 1, port + 1});
    }
    return cords;
}

void PatcherCanvas::dropAudioCords(const std::string& node, int port, const juce::ModifierKeys& mods) {
    const auto cords = cordsForDrop(node, port, mods);
    const bool pairOnly = !mods.isShiftDown() && cords.size() == 2;
    struct Real { std::string src, dst; int srcPort, dstPort; };
    std::vector<Real> real;
    for (const auto& [outlet, inlet] : cords) {
        Real r;
        realPort(dragNode_, outlet, true, false, r.src, r.srcPort);
        realPort(node, inlet, false, false, r.dst, r.dstPort);
        real.push_back(r);
    }
    if (pairOnly)
        for (const auto& c : host_.model().connections)
            if (c.dst == real[1].dst && c.dstInlet == real[1].dstPort) { real.pop_back(); break; }
    host_.beginTransaction();
    for (const auto& r : real) host_.connect(r.src, r.srcPort, r.dst, r.dstPort);
    host_.endTransaction();
}

}
