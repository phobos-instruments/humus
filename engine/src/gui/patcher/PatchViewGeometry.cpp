// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatchView.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "core/packs/Categories.h"
#include "hum/Organism.h"
#include "core/graph/PodModel.h"
#include "gui/app/AppSettings.h"

namespace hum {

std::vector<PatchView::DisplayNode> PatchView::displayNodes() const {
    std::vector<DisplayNode> out;
    for (auto& c : src_.model().organisms)
        if (!hiddenInPatch(c) && pods::inScope(c.name, scope_)) out.push_back({c.name, false});
    for (auto& p : pods::podsIn(src_.model(), scope_)) out.push_back({p, true});
    return out;
}

bool PatchView::isPodBox(const std::string& name) const {
    return pods::parentOf(name) == scope_ && pods::isPod(src_.model(), name);
}

void PatchView::portCounts(const std::string& name, int& ins, int& outs) const {
    if (isPodBox(name)) {
        ins = (int) pods::inletRefs(src_.model(), name).size();
        outs = (int) pods::outletRefs(src_.model(), name).size();
    } else {
        auto& h = const_cast<PatchViewSource&>(src_);
        ins = h.inletsOf(name);
        outs = h.outletsOf(name);
        if (!scope_.empty() && pods::parentOf(name) == scope_)
            if (const auto* cm = src_.model().byName(name)) {
                if (pods::isInletClass(cm->displayClass)) ins = 0;
                else if (pods::isOutletClass(cm->displayClass)) outs = 0;
            }
    }
}

void PatchView::midiPortCounts(const std::string& name, int& ins, int& outs) const {
    if (isPodBox(name)) {
        ins = (int) pods::midiInletRefs(src_.model(), name).size();
        outs = (int) pods::midiOutletRefs(src_.model(), name).size();
        return;
    }
    auto& h = const_cast<PatchViewSource&>(src_);
    ins = h.midiInletsOf(name);
    outs = h.midiOutletsOf(name);
    if (!scope_.empty() && pods::parentOf(name) == scope_)
        if (const auto* cm = src_.model().byName(name)) {
            if (pods::isInletClass(cm->displayClass)) ins = 0;
            else if (pods::isOutletClass(cm->displayClass)) outs = 0;
        }
}

void PatchView::videoPortCounts(const std::string& name, int& ins, int& outs) const {
    if (isPodBox(name)) {
        ins = (int) pods::videoPortRefs(src_.model(), name, true).size();
        outs = (int) pods::videoPortRefs(src_.model(), name, false).size();
        return;
    }
    auto& h = const_cast<PatchViewSource&>(src_);
    ins = h.videoInletsOf(name);
    outs = h.videoOutletsOf(name);
    if (!scope_.empty() && pods::parentOf(name) == scope_)
        if (const auto* cm = src_.model().byName(name)) {
            if (pods::isInletClass(cm->displayClass)) ins = 0;
            else if (pods::isOutletClass(cm->displayClass)) outs = 0;
        }
}

bool PatchView::mapEndpoint(const std::string& node, int port, bool isDstSide,
                                pods::Domain dom, std::string& dispNode, int& dispPort) const {
    const auto s = pods::surfaceOf(src_.model(), node, port, isDstSide, scope_, dom);
    if (s.kind == pods::Surface::None) return false;
    if (s.kind == pods::Surface::Self) {
        if (auto* m = src_.model().byName(node); m && hiddenInPatch(*m)) return false;
        dispNode = node;
        dispPort = port;
        return true;
    }
    dispNode = s.box;
    dispPort = s.pin;
    return true;
}

void PatchView::realPort(const std::string& dispNode, int port, bool isOutlet,
                             pods::Domain dom, std::string& realNode, int& realPort) const {
    if (isPodBox(dispNode)) {
        const auto list =
            dom == pods::Domain::Midi
                ? (isOutlet ? pods::midiOutletRefs(src_.model(), dispNode)
                            : pods::midiInletRefs(src_.model(), dispNode))
            : dom == pods::Domain::Video
                ? pods::videoPortRefs(src_.model(), dispNode, !isOutlet)
                : (isOutlet ? pods::outletRefs(src_.model(), dispNode)
                            : pods::inletRefs(src_.model(), dispNode));
        if (port >= 0 && port < (int) list.size()) {
            realNode = list[(size_t) port].node;
            realPort = list[(size_t) port].chan;
            return;
        }
    }
    realNode = dispNode;
    realPort = port;
}

void PatchView::controlPortCounts(const std::string& name, int& ins, int& outs) const {
    ins = outs = 0;
    auto& h = const_cast<PatchViewSource&>(src_);
    ins = h.controlInletsOf(name);
    outs = h.controlOutletsOf(name);
    if (isPodBox(name)) return;
    if (!scope_.empty() && pods::parentOf(name) == scope_)
        if (const auto* cm = src_.model().byName(name)) {
            if (cm->displayClass == pods::kControlInletClass) ins = 0;
            else if (cm->displayClass == pods::kControlOutletClass) outs = 0;
        }
}

int PatchView::nodeWidth(const std::string& name) {
    int ins, outs, mIns, mOuts, vIns, vOuts, cIns, cOuts;
    portCounts(name, ins, outs);
    midiPortCounts(name, mIns, mOuts);
    videoPortCounts(name, vIns, vOuts);
    controlPortCounts(name, cIns, cOuts);
    const int nA = std::max(ins, outs);
    const int nC = std::max(cIns, cOuts);
    const int nM = std::max(mIns, mOuts);
    const int nV = std::max(vIns, vOuts);
    int need = kPortPad * 2 + kPort + std::max(0, nA - 1) * (kPort + kPortGap) + kGroupGap * audioBreaks(name);
    if (nC > 0) need += (nA > 0 ? kMidiGap : 0) + nC * (kPort + kPortGap) - (nA > 0 ? 0 : kPort + kPortGap);
    if (nV > 0) need += (nA > 0 || nC > 0 ? kVideoGap : 0) + nV * (kPort + kPortGap);
    if (nM > 0) need += kMidiGap + nM * (kPort + kPortGap);
    return std::max(kW, need);
}

int PatchView::afterAudioX(const std::string& name, bool outlets) {
    int ins, outs;
    portCounts(name, ins, outs);
    const int nA = outlets ? outs : ins;
    return kPortPad + kPort / 2
           + (nA > 0 ? nA * (kPort + kPortGap) + kMidiGap + kGroupGap * portSide(name, outlets).breaks() : 0);
}

juce::Point<int> PatchView::controlInletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    return {b.getX() + afterAudioX(name, false) + port * (kPort + kPortGap), b.getY()};
}
juce::Point<int> PatchView::controlOutletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    return {b.getX() + afterAudioX(name, true) + port * (kPort + kPortGap), b.getBottom()};
}

const portgroups::Side& PatchView::portSide(const std::string& name, bool outlets) const {
    int ins, outs;
    portCounts(name, ins, outs);
    const auto* cm = src_.model().byName(name);
    const std::string cls = cm != nullptr ? cm->classRaw : std::string();
    auto& cached = portSides_[name];
    if (cached.ins != ins || cached.outs != outs || cached.cls != cls || !cached.resolved) {
        auto* live = isPodBox(name) ? nullptr : const_cast<PatchViewSource&>(src_).liveOrganism(name);
        const auto* ports = dynamic_cast<const PortNames*>(live);
        cached.ins = ins;
        cached.outs = outs;
        cached.cls = cls;
        cached.resolved = live != nullptr || isPodBox(name);
        cached.in = portgroups::describe(ports, ins, false);
        cached.out = portgroups::describe(ports, outs, true);
    }
    return outlets ? cached.out : cached.in;
}

int PatchView::audioBreaks(const std::string& name) const {
    return std::max(portSide(name, false).breaks(), portSide(name, true).breaks());
}

std::string PatchView::portName(const std::string& node, int port, bool outlet) const {
    return portSide(node, outlet).name(port);
}

juce::Rectangle<int> PatchView::nodeBounds(const std::string& name) {
    auto p = src_.position(name);
    return {p.x, p.y, nodeWidth(name), kH};
}

juce::Point<int> PatchView::inletPos(const std::string& name, int inlet, int count) {
    juce::ignoreUnused(count);
    auto b = nodeBounds(name);
    return {b.getX() + kPortPad + kPort / 2 + inlet * (kPort + kPortGap)
                + kGroupGap * portSide(name, false).breaksAt(inlet), b.getY()};
}
juce::Point<int> PatchView::outletPos(const std::string& name, int outlet, int count) {
    juce::ignoreUnused(count);
    auto b = nodeBounds(name);
    return {b.getX() + kPortPad + kPort / 2 + outlet * (kPort + kPortGap)
                + kGroupGap * portSide(name, true).breaksAt(outlet), b.getBottom()};
}

int PatchView::videoBlockWidth(const std::string& name, bool outlets) {
    int vIns, vOuts;
    videoPortCounts(name, vIns, vOuts);
    const int nV = outlets ? vOuts : vIns;
    return nV > 0 ? nV * (kPort + kPortGap) + kMidiGap : 0;
}

juce::Point<int> PatchView::midiInletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int mIns, mOuts;
    midiPortCounts(name, mIns, mOuts);
    return {b.getRight() - kPortPad - kPort / 2 - videoBlockWidth(name, false)
                - (mIns - 1 - port) * (kPort + kPortGap),
            b.getY()};
}
juce::Point<int> PatchView::midiOutletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int mIns, mOuts;
    midiPortCounts(name, mIns, mOuts);
    return {b.getRight() - kPortPad - kPort / 2 - videoBlockWidth(name, true)
                - (mOuts - 1 - port) * (kPort + kPortGap),
            b.getBottom()};
}

juce::Point<int> PatchView::videoInletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int vIns, vOuts;
    videoPortCounts(name, vIns, vOuts);
    return {b.getRight() - kPortPad - kPort / 2 - (vIns - 1 - port) * (kPort + kPortGap), b.getY()};
}
juce::Point<int> PatchView::videoOutletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int vIns, vOuts;
    videoPortCounts(name, vIns, vOuts);
    return {b.getRight() - kPortPad - kPort / 2 - (vOuts - 1 - port) * (kPort + kPortGap),
            b.getBottom()};
}

}
