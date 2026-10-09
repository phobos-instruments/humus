// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"

#include "gui/style/HandCursors.h"

namespace hum {

namespace {

constexpr int kLiftDistance = 4;

pods::Domain domainOf(bool midi, bool video) {
    return midi ? pods::Domain::Midi : video ? pods::Domain::Video : pods::Domain::Audio;
}

}

std::vector<PatcherCanvas::ShownCord> PatcherCanvas::shownCords(bool midi, bool video, bool control) const {
    std::vector<ShownCord> out;
    if (control) {
        for (const auto& c : host_.controlCordsInScope(scope_))
            out.push_back({Edge{c.src, c.srcOutlet, c.dst, c.dstInlet, false, false, true},
                           c.src, c.srcOutlet, c.dst, c.dstInlet, 0});
        return out;
    }
    const auto& list = midi ? host_.model().midiConnections
                     : video ? host_.model().videoConnections : host_.model().connections;
    for (const auto& c : list) {
        ShownCord s{Edge{c.src, c.srcOutlet, c.dst, c.dstInlet, midi, video}, {}, 0, {}, 0, c.midiChannel};
        if (!mapEndpoint(c.src, c.srcOutlet, false, domainOf(midi, video), s.srcNode, s.srcPort)
            || !mapEndpoint(c.dst, c.dstInlet, true, domainOf(midi, video), s.dstNode, s.dstPort)
            || s.srcNode == s.dstNode)
            continue;
        out.push_back(std::move(s));
    }
    return out;
}

std::vector<PatcherCanvas::ShownCord> PatcherCanvas::cordsAt(const PortRef& at) const {
    std::vector<ShownCord> out;
    for (auto& s : shownCords(at.midi, at.video, at.control)) {
        const bool here = at.outlet ? s.srcNode == at.node && s.srcPort == at.port
                                    : s.dstNode == at.node && s.dstPort == at.port;
        if (here) out.push_back(std::move(s));
    }
    return out;
}

bool PatcherCanvas::grabbable(const PortRef& at, const juce::ModifierKeys& mods) const {
    return (!at.outlet || mods.isCommandDown()) && !cordsAt(at).empty();
}

bool PatcherCanvas::beginGrab(const PortRef& at, juce::Point<int> p, const juce::ModifierKeys& mods) {
    if (!grabbable(at, mods)) return false;
    auto leads = cordsAt(at);
    if (!at.outlet) leads.erase(leads.begin(), leads.end() - 1);
    const bool pairs = !at.midi && !at.video && !at.control;
    const auto all = pairs ? shownCords(false, false, false) : std::vector<ShownCord>{};
    carried_.clear();
    for (const auto& lead : leads) {
        carried_.push_back({lead, 0});
        if (lead.srcPort % 2 != 0 || lead.dstPort % 2 != 0) continue;
        for (const auto& s : all)
            if (s.srcNode == lead.srcNode && s.srcPort == lead.srcPort + 1 && s.dstNode == lead.dstNode
                && s.dstPort == lead.dstPort + 1) {
                carried_.push_back({s, 1});
                break;
            }
    }
    grabFrom_ = at;
    grabStart_ = p;
    grabLifted_ = false;
    cordEnd_ = p;
    cordTarget_ = false;
    drag_ = Drag::Grab;
    setMouseCursor(grabbingHandCursor());
    return true;
}

bool PatcherCanvas::landingAt(juce::Point<int> p, PortRef& out) {
    return hitPort(p, out.node, out.port, out.outlet, out.midi, out.video, out.control) && fitsDrag(out);
}

bool PatcherCanvas::fitsDrag(const PortRef& t) const {
    if (drag_ == Drag::Cord)
        return !t.outlet && t.node != dragNode_ && t.midi == cordMidi_ && t.video == cordVideo_
               && t.control == cordControl_;
    if (drag_ != Drag::Grab || t.outlet != grabFrom_.outlet || t.midi != grabFrom_.midi
        || t.video != grabFrom_.video || t.control != grabFrom_.control)
        return false;
    for (const auto& c : carried_)
        if ((t.outlet ? c.cord.dstNode : c.cord.srcNode) == t.node) return false;
    return true;
}

bool PatcherCanvas::isDragAnchor(const PortRef& t) const {
    if (drag_ == Drag::Cord)
        return t.outlet && t.node == dragNode_ && t.port == cordOutlet_ && t.midi == cordMidi_
               && t.video == cordVideo_ && t.control == cordControl_;
    if (t.outlet == grabFrom_.outlet || t.midi != grabFrom_.midi || t.video != grabFrom_.video
        || t.control != grabFrom_.control)
        return false;
    for (const auto& c : carried_)
        if (t.outlet ? c.cord.srcNode == t.node && c.cord.srcPort == t.port
                     : c.cord.dstNode == t.node && c.cord.dstPort == t.port)
            return true;
    return false;
}

void PatcherCanvas::noteRefusal(juce::Point<int> p) {
    PortRef t;
    refused_ = hitPort(p, t.node, t.port, t.outlet, t.midi, t.video, t.control) && refusesDuringDrag(t);
    if (refused_) refusedAt_ = t;
    setMouseCursor(refused_ ? notAllowedCursor()
                   : drag_ == Drag::Grab ? grabbingHandCursor()
                                         : juce::MouseCursor(juce::MouseCursor::PointingHandCursor));
}

void PatcherCanvas::dragGrab(juce::Point<int> p) {
    cordEnd_ = p;
    if (!grabLifted_ && p.getDistanceFrom(grabStart_) < kLiftDistance) return;
    grabLifted_ = true;
    cordTarget_ = landingAt(p, grabTarget_);
    noteRefusal(p);
    refresh();
}

void PatcherCanvas::dropGrab(juce::Point<int> p, const juce::ModifierKeys& mods) {
    if (carried_.empty()) return;
    if (!grabLifted_) {
        cordSelected_ = true;
        selectedCord_ = carried_.front().cord.real;
        selection_.clear();
        primary_.clear();
        notifySelection();
    } else if (PortRef to; landingAt(p, to)) {
        moveCarried(to, mods);
    } else if (PortRef any; !hitPort(p, any.node, any.port, any.outlet, any.midi, any.video, any.control)
                            && hitNode(p).empty()) {
        host_.beginTransaction();
        for (const auto& c : carried_) removeCord(c.cord.real);
        host_.endTransaction();
    }
    carried_.clear();
}

void PatcherCanvas::cancelGrab() {
    carried_.clear();
    drag_ = Drag::None;
    cordTarget_ = false;
    refused_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    refresh();
}

bool PatcherCanvas::pairFits(const PortRef& at) const {
    if (at.port % 2 != 0) return false;
    int ins = 0, outs = 0;
    portCounts(at.node, ins, outs);
    if (at.port + 1 >= (at.outlet ? outs : ins)) return false;
    if (at.outlet) return true;
    for (const auto& s : cordsAt({at.node, at.port + 1, false}))
        if (!isCarried(s.real)) return false;
    return true;
}

void PatcherCanvas::moveCarried(const PortRef& to, const juce::ModifierKeys& mods) {
    if (to.node == grabFrom_.node && to.port == grabFrom_.port) return;
    bool hasPartner = false;
    for (const auto& c : carried_) hasPartner = hasPartner || c.shift > 0;
    const bool pair = hasPartner && !mods.isAltDown() && pairFits(to);
    host_.beginTransaction();
    for (const auto& c : carried_) {
        if (c.shift > 0 && !pair) continue;
        removeCord(c.cord.real);
        reconnect(c.cord, to, to.port + c.shift);
    }
    host_.endTransaction();
}

void PatcherCanvas::reconnect(const ShownCord& c, const PortRef& to, int port) {
    std::string src = c.real.src, dst = c.real.dst;
    int srcPort = c.real.srcOutlet, dstPort = c.real.dstInlet;
    if (to.control) {
        (to.outlet ? src : dst) = to.node;
        (to.outlet ? srcPort : dstPort) = port;
        host_.connectControl(src, srcPort, dst, dstPort);
        return;
    }
    if (to.outlet) realPort(to.node, port, true, domainOf(to.midi, to.video), src, srcPort);
    else realPort(to.node, port, false, domainOf(to.midi, to.video), dst, dstPort);
    if (to.midi) {
        host_.connectMidi(src, srcPort, dst, dstPort);
        if (c.channel >= 1) host_.setMidiCordChannel(src, srcPort, dst, dstPort, c.channel);
    } else if (to.video) {
        host_.connectVideo(src, srcPort, dst, dstPort);
    } else {
        for (const auto& m : host_.model().connections)
            if (m.src == src && m.srcOutlet == srcPort && m.dst == dst && m.dstInlet == dstPort) return;
        host_.connect(src, srcPort, dst, dstPort);
    }
}

void PatcherCanvas::removeCord(const Edge& e) {
    if (e.control) host_.disconnectControl(e.src, e.srcOutlet, e.dst, e.dstInlet);
    else if (e.midi) host_.removeMidiConnection(e.src, e.srcOutlet, e.dst, e.dstInlet);
    else if (e.video) host_.removeVideoConnection(e.src, e.srcOutlet, e.dst, e.dstInlet);
    else host_.removeConnection(e.src, e.srcOutlet, e.dst, e.dstInlet);
}

bool PatcherCanvas::isCarried(const Edge& e) const {
    if (drag_ != Drag::Grab || !grabLifted_) return false;
    for (const auto& c : carried_)
        if (c.cord.real == e) return true;
    return false;
}

std::vector<PatcherCanvas::CarriedLine> PatcherCanvas::carriedLines() {
    std::vector<CarriedLine> out;
    if (drag_ != Drag::Grab || !grabLifted_) return out;
    for (const auto& c : carried_) {
        PortRef anchor = grabFrom_;
        anchor.outlet = !grabFrom_.outlet;
        anchor.node = grabFrom_.outlet ? c.cord.dstNode : c.cord.srcNode;
        anchor.port = grabFrom_.outlet ? c.cord.dstPort : c.cord.srcPort;
        const auto fixed = portPos(anchor).toFloat();
        const auto loose = (cordEnd_ + juce::Point<int>((kPort + kPortGap) * c.shift, 0)).toFloat();
        out.push_back(grabFrom_.outlet ? CarriedLine{loose, fixed, c.cord.real}
                                       : CarriedLine{fixed, loose, c.cord.real});
    }
    return out;
}

juce::Point<int> PatcherCanvas::portPos(const PortRef& at) {
    if (at.control) return at.outlet ? controlOutletPos(at.node, at.port) : controlInletPos(at.node, at.port);
    if (at.midi) return at.outlet ? midiOutletPos(at.node, at.port) : midiInletPos(at.node, at.port);
    if (at.video) return at.outlet ? videoOutletPos(at.node, at.port) : videoInletPos(at.node, at.port);
    int ins = 0, outs = 0;
    portCounts(at.node, ins, outs);
    return at.outlet ? outletPos(at.node, at.port, outs) : inletPos(at.node, at.port, ins);
}

void PatcherCanvas::modifierKeysChanged(const juce::ModifierKeys& mods) {
    if (drag_ != Drag::None || !portHover_) return;
    const PortRef at{hoverPortNode_, hoverPort_, hoverPortOut_, hoverPortMidi_, hoverPortVideo_, hoverPortControl_};
    setMouseCursor(grabbable(at, mods) ? juce::MouseCursor::DraggingHandCursor
                                       : juce::MouseCursor::PointingHandCursor);
}

}
