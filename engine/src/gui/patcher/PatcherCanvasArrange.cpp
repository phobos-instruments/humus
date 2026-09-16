// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"

#include <map>
#include <vector>

#include "core/graph/GraphLayout.h"

namespace hum {

void PatcherCanvas::autoArrange() {
    const auto disp = displayNodes();
    if (disp.empty()) return;

    std::vector<LayoutNode> nodes;
    std::map<std::string, int> index;
    for (const auto& d : disp) {
        index[d.name] = (int) nodes.size();
        nodes.push_back({nodeWidth(d.name), kH});
    }
    std::vector<LayoutEdge> edges;
    auto addEdges = [&](const std::vector<ConnectionModel>& cords, bool midi) {
        for (const auto& c : cords) {
            std::string sn, dn;
            int sp, dp;
            if (!mapEndpoint(c.src, c.srcOutlet, false, midi, sn, sp)
                || !mapEndpoint(c.dst, c.dstInlet, true, midi, dn, dp) || sn == dn)
                continue;
            const auto si = index.find(sn), di = index.find(dn);
            if (si != index.end() && di != index.end())
                edges.push_back({si->second, di->second, sp, dp});
        }
    };
    addEdges(host_.model().connections, false);
    addEdges(host_.model().midiConnections, true);
    for (const auto& c : host_.model().videoConnections) {
        const auto si = index.find(c.src), di = index.find(c.dst);
        if (si != index.end() && di != index.end() && si->second != di->second)
            edges.push_back({si->second, di->second});
    }

    LayoutMetrics metrics;
    metrics.marginY = 24 + (scope_.empty() ? 16 : kCrumbH + 16);

    const auto placed = layoutFlowGraph(nodes, edges, metrics);

    int minX = placed[0].first, maxX = placed[0].first + nodes[0].width;
    for (size_t i = 1; i < placed.size(); ++i) {
        minX = std::min(minX, placed[i].first);
        maxX = std::max(maxX, placed[i].first + nodes[i].width);
    }
    int visW = getLocalBounds().getWidth();
    if (auto* vp = dynamic_cast<const juce::Viewport*>(getParentComponent()))
        visW = vp->getViewArea().getWidth();
    const int dx = std::max(0, (visW - (maxX - minX)) / 2 - minX);

    host_.pushUndo();
    for (size_t i = 0; i < disp.size(); ++i)
        host_.setPosition(disp[i].name, {placed[i].first + dx, placed[i].second});
    host_.markDirty();
    refresh();
}

}
