// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"
#include <algorithm>
#include "core/graph/AutoWire.h"
#include "core/packs/Categories.h"
#include "core/graph/PodModel.h"
#include "gui/common/Localisation.h"

namespace hum {

namespace {
bool segmentHitsRect(juce::Point<int> a, juce::Point<int> b, juce::Rectangle<int> r) {
    for (int i = 0; i <= 24; ++i) {
        float t = i / 24.0f;
        if (r.contains(juce::Point<int>((int) (a.x + t * (b.x - a.x)),
                                        (int) (a.y + t * (b.y - a.y)))))
            return true;
    }
    return false;
}
}

void PatcherCanvas::updateDragHints() {
    spliceBundle_.clear();
    proxConnections_.clear();
    if (selection_.size() != 1 || primary_.empty()) return;

    const std::string node = primary_;
    int ins, outs, mIns, mOuts, vIns, vOuts, cIns, cOuts;
    portCounts(node, ins, outs);
    midiPortCounts(node, mIns, mOuts);
    videoPortCounts(node, vIns, vOuts);
    controlPortCounts(node, cIns, cOuts);
    if (ins <= 0 && outs <= 0 && mIns <= 0 && mOuts <= 0 && vIns <= 0 && vOuts <= 0
        && cIns <= 0 && cOuts <= 0)
        return;

    auto wired = [&](const std::string& disp) {
        if (isPodBox(disp)) {
            auto touched = [&](const std::vector<pods::PortRef>& refs, bool inletSide,
                               bool midi) {
                const auto& cords = midi ? host_.model().midiConnections
                                         : host_.model().connections;
                for (const auto& r : refs)
                    for (const auto& c : cords)
                        if (inletSide ? c.dst == r.node : c.src == r.node) return true;
                return false;
            };
            const auto& m = host_.model();
            return touched(pods::inletRefs(m, disp), true, false)
                || touched(pods::outletRefs(m, disp), false, false)
                || touched(pods::midiInletRefs(m, disp), true, true)
                || touched(pods::midiOutletRefs(m, disp), false, true);
        }
        for (auto& c : host_.model().connections)
            if (c.src == disp || c.dst == disp) return true;
        for (auto& c : host_.model().midiConnections)
            if (c.src == disp || c.dst == disp) return true;
        for (auto& c : host_.model().videoConnections)
            if (c.src == disp || c.dst == disp) return true;
        for (const auto& c : host_.controlCordsInScope(scope_))
            if (c.src == disp || c.dst == disp) return true;
        return false;
    };
    if (wired(node)) return;

    auto nb = nodeBounds(node);

    if (ins > 0 && outs > 0) {
        std::string bsrc, bdst;
        for (auto& c : host_.model().connections) {
            if (c.src == node || c.dst == node) continue;
            if (!pods::inScope(c.src, scope_) || !pods::inScope(c.dst, scope_)) continue;
            int so = host_.outletsOf(c.src), si = host_.inletsOf(c.dst);
            if (so == 0 || si == 0) continue;
            if (segmentHitsRect(outletPos(c.src, c.srcOutlet, so),
                                inletPos(c.dst, c.dstInlet, si), nb)) {
                bsrc = c.src; bdst = c.dst; break;
            }
        }
        if (!bsrc.empty()) {
            for (auto& c : host_.model().connections)
                if (c.src == bsrc && c.dst == bdst)
                    spliceBundle_.push_back({c.src, c.srcOutlet, c.dst, c.dstInlet});
            std::sort(spliceBundle_.begin(), spliceBundle_.end(),
                      [](const Edge& a, const Edge& b) { return a.dstInlet < b.dstInlet; });
            return;
        }
    }

    const int kGap = 32;
    std::string bestName;
    bool bestNodeFeeds = false;
    int bestDist = 1 << 30;
    for (auto& o : displayNodes()) {
        if (o.name == node) continue;
        auto ob = nodeBounds(o.name);
        const bool overlapX = nb.getRight() > ob.getX() + 8 && nb.getX() < ob.getRight() - 8;
        if (!overlapX) continue;
        int oIns, oOuts, oMIns, oMOuts, oVIns, oVOuts, oCIns, oCOuts;
        portCounts(o.name, oIns, oOuts);
        midiPortCounts(o.name, oMIns, oMOuts);
        videoPortCounts(o.name, oVIns, oVOuts);
        controlPortCounts(o.name, oCIns, oCOuts);
        if ((outs > 0 && oIns > 0) || (mOuts > 0 && oMIns > 0)
            || (vOuts > 0 && oVIns > 0) || (cOuts > 0 && oCIns > 0)) {
            const int d = nb.getBottom() - ob.getY();
            if (d >= -kGap && d <= ob.getHeight() / 2 && std::abs(d) < bestDist) {
                bestDist = std::abs(d);
                bestName = o.name;
                bestNodeFeeds = true;
            }
        }
        if ((ins > 0 && oOuts > 0) || (mIns > 0 && oMOuts > 0)
            || (vIns > 0 && oVOuts > 0) || (cIns > 0 && oCOuts > 0)) {
            const int d = ob.getBottom() - nb.getY();
            if (d >= -kGap && d <= ob.getHeight() / 2 && std::abs(d) < bestDist) {
                bestDist = std::abs(d);
                bestName = o.name;
                bestNodeFeeds = false;
            }
        }
    }
    if (bestName.empty()) return;
    int oIns, oOuts, oMIns, oMOuts, oVIns, oVOuts, oCIns, oCOuts;
    portCounts(bestName, oIns, oOuts);
    midiPortCounts(bestName, oMIns, oMOuts);
    videoPortCounts(bestName, oVIns, oVOuts);
    controlPortCounts(bestName, oCIns, oCOuts);
    auto offerControl = [&](const std::string& srcD, int so, const std::string& dstD, int di) {
        for (const auto& c : host_.controlCordsInScope(scope_))
            if (c.src == srcD && c.srcOutlet == so && c.dst == dstD && c.dstInlet == di) return;
        proxConnections_.push_back({srcD, so, dstD, di, false, false, true});
    };
    auto socketsInUse = [&](const std::string& dstD, int count) {
        std::vector<char> busy((size_t) std::max(0, count), 0);
        for (const auto& c : host_.controlCordsInScope(scope_))
            if (c.dst == dstD && c.dstInlet >= 0 && c.dstInlet < count) busy[(size_t) c.dstInlet] = 1;
        return busy;
    };
    auto resolve = [&](const std::string& disp, int port, bool isOutlet, bool midi,
                       bool video, std::string& outNode, int& outPort) {
        if (video) { outNode = disp; outPort = port; return; }
        realPort(disp, port, isOutlet, midi, outNode, outPort);
    };
    auto offer = [&](const std::string& srcD, int so, const std::string& dstD, int di,
                     bool midi, bool video) {
        std::string rs, rd;
        int rsp = 0, rdp = 0;
        resolve(srcD, so, true, midi, video, rs, rsp);
        resolve(dstD, di, false, midi, video, rd, rdp);
        const bool dup = video ? host_.isVideoConnected(rs, rsp, rd, rdp)
                       : midi  ? host_.isMidiConnected(rs, rsp, rd, rdp)
                               : host_.isConnected(rs, rsp, rd, rdp);
        if (!dup) proxConnections_.push_back({srcD, so, dstD, di, midi, video});
    };
    auto inletsInUse = [&](const std::string& dstD, int count, bool midi, bool video) {
        std::vector<char> busy((size_t) std::max(0, count), 0);
        const auto& cords = video ? host_.model().videoConnections
                          : midi  ? host_.model().midiConnections
                                  : host_.model().connections;
        for (int i = 0; i < count; ++i) {
            std::string rd;
            int rdp = 0;
            resolve(dstD, i, false, midi, video, rd, rdp);
            for (const auto& c : cords)
                if (c.dst == rd && c.dstInlet == rdp) { busy[(size_t) i] = 1; break; }
        }
        return busy;
    };
    if (bestNodeFeeds) {
        const int nA = std::min(outs, oIns);
        const int baseA = firstFreeInletRun(inletsInUse(bestName, oIns, false, false), nA);
        for (int k = 0; k < nA; ++k) offer(node, k, bestName, baseA + k, false, false);
        const int nM = std::min(mOuts, oMIns);
        const int baseM = firstFreeInletRun(inletsInUse(bestName, oMIns, true, false), nM);
        for (int k = 0; k < nM; ++k) offer(node, k, bestName, baseM + k, true, false);
        const int nV = std::min(vOuts, oVIns);
        const int baseV = firstFreeInletRun(inletsInUse(bestName, oVIns, false, true), nV);
        for (int k = 0; k < nV; ++k) offer(node, k, bestName, baseV + k, false, true);
        const int nC = std::min(cOuts, oCIns);
        const int baseC = firstFreeInletRun(socketsInUse(bestName, oCIns), nC);
        for (int k = 0; k < nC; ++k) offerControl(node, k, bestName, baseC + k);
    } else {
        for (int k = 0; k < std::min(oOuts, ins); ++k) offer(bestName, k, node, k, false, false);
        for (int k = 0; k < std::min(oMOuts, mIns); ++k) offer(bestName, k, node, k, true, false);
        for (int k = 0; k < std::min(oVOuts, vIns); ++k) offer(bestName, k, node, k, false, true);
        for (int k = 0; k < std::min(oCOuts, cIns); ++k) offerControl(bestName, k, node, k);
    }
}

void PatcherCanvas::commitDragHints() {
    const std::string node = primary_;
    if (!spliceBundle_.empty()) {
        host_.beginTransaction();
        int ins, outs;
        portCounts(node, ins, outs);
        int i = 0;
        for (auto& e : spliceBundle_) {
            std::string ri, ro;
            int rip = 0, rop = 0;
            realPort(node, std::min(i, ins - 1), false, false, ri, rip);
            realPort(node, std::min(i, outs - 1), true, false, ro, rop);
            host_.removeConnection(e.src, e.srcOutlet, e.dst, e.dstInlet);
            host_.connect(e.src, e.srcOutlet, ri, rip);
            host_.connect(ro, rop, e.dst, e.dstInlet);
            ++i;
        }
        host_.endTransaction();
    } else if (!proxConnections_.empty()) {
        host_.beginTransaction();
        for (auto& e : proxConnections_) {
            if (e.control) {
                host_.connectControl(e.src, e.srcOutlet, e.dst, e.dstInlet);
                continue;
            }
            if (e.video) {
                host_.connectVideo(e.src, e.srcOutlet, e.dst, e.dstInlet);
                continue;
            }
            std::string rs, rd;
            int rsp = 0, rdp = 0;
            realPort(e.src, e.srcOutlet, true, e.midi, rs, rsp);
            realPort(e.dst, e.dstInlet, false, e.midi, rd, rdp);
            if (e.midi) host_.connectMidi(rs, rsp, rd, rdp);
            else host_.connect(rs, rsp, rd, rdp);
        }
        host_.endTransaction();
    }
}

}
