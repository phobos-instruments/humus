// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatcherCanvas.h"

#include <algorithm>

#include "core/graph/AutoWire.h"
#include "core/packs/Categories.h"
#include "core/graph/PodModel.h"
#include "gui/common/Localisation.h"

namespace hum {

static bool hidden(const OrganismModel& c) { return isHiddenOrganism(c.displayClass); }

bool PatcherCanvas::beginPan(const juce::MouseEvent& e) {
    auto* vp = findParentComponentOfClass<juce::Viewport>();
    if (vp == nullptr) return false;
    drag_ = Drag::Pan;
    panMouseAnchor_ = e.getEventRelativeTo(vp).getPosition();
    panViewAnchor_ = vp->getViewPosition();
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    return true;
}

void PatcherCanvas::mouseDown(const juce::MouseEvent& e) {
    auto p = modelPos(e);
    if (e.mods.isMiddleButtonDown() && beginPan(e)) return;
    grabKeyboardFocus();

    if (e.mods.isPopupMenu()) {
        if (auto n = hitNode(p); !n.empty()) {
            if (!selection_.count(n)) { select(n); notifySelection(); refresh(); }
            if (isPodBox(n)) showPodMenu(n, e.getScreenPosition());
            else             showNodeMenu(n, e.getScreenPosition());
        } else if (Edge ce; hitCord(p, ce)) {
            std::string sn, dn;
            int sp, dp;
            juce::Point<int> a, b;
            if (mapEndpoint(ce.src, ce.srcOutlet, false, ce.midi, sn, sp)
                && mapEndpoint(ce.dst, ce.dstInlet, true, ce.midi, dn, dp)) {
                int outs, ins, tmp;
                if (ce.midi) {
                    midiPortCounts(sn, tmp, outs);
                    midiPortCounts(dn, ins, tmp);
                    a = midiOutletPos(sn, sp);
                    b = midiInletPos(dn, dp);
                } else {
                    portCounts(sn, tmp, outs);
                    portCounts(dn, ins, tmp);
                    a = outletPos(sn, sp, outs);
                    b = inletPos(dn, dp, ins);
                }
            }
            showCordMenu(ce, (a + b) / 2, e.getScreenPosition());
        } else {
            showAddMenu(p, e.getScreenPosition());
        }
        return;
    }

    std::string node; int port; bool isOutlet, isMidi, isVideo, isControl;
    if (hitPort(p, node, port, isOutlet, isMidi, isVideo, isControl)) {
        if (beginGrab({node, port, isOutlet, isMidi, isVideo, isControl}, p, e.mods)) return;
        if (isOutlet) {
            drag_ = Drag::Cord; dragNode_ = node; cordOutlet_ = port;
            cordMidi_ = isMidi; cordVideo_ = isVideo; cordControl_ = isControl; cordEnd_ = p;
            return;
        }
    }

    Edge ce;
    if (hitCord(p, ce)) {
        cordSelected_ = true; selectedCord_ = ce;
        selection_.clear(); primary_.clear(); notifySelection();
        refresh();
        return;
    }
    cordSelected_ = false;

    std::string n = hitNode(p);
    const bool additive = e.mods.isShiftDown() || e.mods.isCommandDown();

    if (n.empty()) {
        if (e.getNumberOfClicks() >= 2) {
            drag_ = Drag::None;
            openQuickAdd(p, e.getScreenPosition());
            return;
        }
        if (e.mods.isAltDown() && beginPan(e)) return;
        if (!additive) { selection_.clear(); primary_.clear(); notifySelection(); }
        drag_ = Drag::Select; dragStart_ = p; selectRect_ = {p.x, p.y, 0, 0};
        refresh();
        return;
    }

    if (additive) {
        if (selection_.count(n)) selection_.erase(n); else selection_.insert(n);
        primary_ = selection_.empty() ? "" : n;
    } else {
        if (!selection_.count(n)) { selection_.clear(); selection_.insert(n); }
        primary_ = n;
    }
    notifySelection();
    if (e.getNumberOfClicks() >= 2) {
        if (isPodBox(n)) { enterPod(n); return; }
        if (onActivate) onActivate(n);
    }

    drag_ = Drag::Move; dragStart_ = p;
    dragLead_ = n;
    origPos_.clear();
    for (auto& s : selection_) origPos_[s] = host_.position(s);
    refresh();
}

void PatcherCanvas::mouseDrag(const juce::MouseEvent& e) {
    if (drag_ == Drag::Pan) {
        if (auto* vp = findParentComponentOfClass<juce::Viewport>())
            vp->setViewPosition(panViewAnchor_
                                - (e.getEventRelativeTo(vp).getPosition() - panMouseAnchor_));
        return;
    }
    if (drag_ != Drag::None) autoScrollWhileDragging(e);
    if (drag_ == Drag::Move) {
        auto d = modelPos(e) - dragStart_;
        alignGuideX_ = alignGuideY_ = -1;
        if (!e.mods.isAltDown() && origPos_.count(dragLead_) > 0) {
            juce::Point<int> lead = origPos_[dragLead_] + d;
            constexpr int kReach = 6;
            int bestDx = kReach + 1, bestDy = kReach + 1, snapX = 0, snapY = 0;
            for (auto& other : displayNodes()) {
                if (selection_.count(other.name) > 0) continue;
                const auto ob = nodeBounds(other.name);
                if (std::abs(ob.getX() - lead.x) < bestDx) {
                    bestDx = std::abs(ob.getX() - lead.x);
                    snapX = ob.getX();
                }
                if (std::abs(ob.getY() - lead.y) < bestDy) {
                    bestDy = std::abs(ob.getY() - lead.y);
                    snapY = ob.getY();
                }
            }
            if (bestDx <= kReach) { lead.x = snapX; alignGuideX_ = snapX; }
            else lead.x = ((lead.x + kGrid / 2) / kGrid) * kGrid;
            if (bestDy <= kReach) { lead.y = snapY; alignGuideY_ = snapY; }
            else lead.y = ((lead.y + kGrid / 2) / kGrid) * kGrid;
            d = lead - origPos_[dragLead_];
        }
        for (auto& kv : origPos_) host_.setPosition(kv.first, kv.second + d);
        updateDragHints();
        refresh();
    } else if (drag_ == Drag::Cord) {
        cordEnd_ = modelPos(e);
        PortRef t;
        cordTarget_ = hitPort(cordEnd_, t.node, t.port, t.outlet, t.midi, t.video, t.control) && fitsDrag(t);
        if (cordTarget_) { cordTargetNode_ = t.node; cordTargetPort_ = t.port; }
        noteRefusal(cordEnd_);
        refresh();
    } else if (drag_ == Drag::Grab) {
        dragGrab(modelPos(e));
    } else if (drag_ == Drag::Select) {
        selectRect_ = juce::Rectangle<int>(dragStart_, modelPos(e));
        selection_.clear();
        for (auto& d : displayNodes())
            if (selectRect_.intersects(nodeBounds(d.name))) selection_.insert(d.name);
        refresh();
    }
}

void PatcherCanvas::mouseUp(const juce::MouseEvent& e) {
    if (drag_ == Drag::Cord) {
        std::string node; int port; bool isOutlet, isMidi, isVideo, isControl;
        if (hitPort(modelPos(e), node, port, isOutlet, isMidi, isVideo, isControl)
            && fitsDrag({node, port, isOutlet, isMidi, isVideo, isControl})) {
            if (cordControl_) {
                host_.connectControl(dragNode_, cordOutlet_, node, port);
            } else if (cordVideo_) {
                std::string rs, rd;
                int rsp, rdp;
                realPort(dragNode_, cordOutlet_, true, pods::Domain::Video, rs, rsp);
                realPort(node, port, false, pods::Domain::Video, rd, rdp);
                host_.connectVideo(rs, rsp, rd, rdp);
            } else if (cordMidi_) {
                std::string rs, rd;
                int rsp, rdp;
                realPort(dragNode_, cordOutlet_, true, true, rs, rsp);
                realPort(node, port, false, true, rd, rdp);
                host_.connectMidi(rs, rsp, rd, rdp);
            } else {
                dropAudioCords(node, port, e.mods);
            }
        }
    } else if (drag_ == Drag::Select) {
        primary_ = selection_.size() == 1 ? *selection_.begin() : "";
        notifySelection();
    } else if (drag_ == Drag::Move) {
        commitDragHints();
    } else if (drag_ == Drag::Grab) {
        dropGrab(modelPos(e), e.mods);
    }
    if (drag_ == Drag::Pan || drag_ == Drag::Grab || drag_ == Drag::Cord)
        setMouseCursor(juce::MouseCursor::NormalCursor);
    drag_ = Drag::None;
    refused_ = false;
    dragNode_.clear();
    cordTarget_ = false;
    alignGuideX_ = alignGuideY_ = -1;
    spliceBundle_.clear();
    proxConnections_.clear();
    refresh();
}

void PatcherCanvas::mouseMove(const juce::MouseEvent& e) {
    const auto p = modelPos(e);
    bool changed = false;

    std::string node; int port; bool isOutlet, isMidi, isVideo;
    bool isControl = false;
    bool ph = hitPort(p, node, port, isOutlet, isMidi, isVideo, isControl);
    if (ph != portHover_ || (ph && (node != hoverPortNode_ || port != hoverPort_
                                    || isOutlet != hoverPortOut_ || isMidi != hoverPortMidi_
                                    || isVideo != hoverPortVideo_ || isControl != hoverPortControl_))) {
        portHover_ = ph; hoverPortNode_ = node; hoverPort_ = port; hoverPortOut_ = isOutlet;
        hoverPortMidi_ = isMidi;
        hoverPortVideo_ = isVideo;
        hoverPortControl_ = isControl;
        changed = true;
    }

    Edge ce;
    bool h = !ph && hitCord(p, ce);
    if (h != cordHovered_ || (h && !(ce == hoverCord_))) {
        cordHovered_ = h;
        hoverCord_ = ce;
        changed = true;
    }

    const bool grab = ph && grabbable({node, port, isOutlet, isMidi, isVideo, isControl}, e.mods);
    setMouseCursor(grab ? juce::MouseCursor::DraggingHandCursor
                   : (ph || h) ? juce::MouseCursor::PointingHandCursor
                   : (e.mods.isAltDown() && hitNode(p).empty())
                       ? juce::MouseCursor::DraggingHandCursor
                       : juce::MouseCursor::NormalCursor);
    if (changed) refresh();
}

namespace {
juce::String arrowTo()   { return juce::String::fromUTF8("\xe2\x86\x92 "); }
juce::String arrowFrom() { return juce::String::fromUTF8("\xe2\x86\x90 "); }
juce::String dash()      { return " - "; }
juce::String chanSuffix(int channel) {
    return channel >= 1 ? juce::String::fromUTF8("  \xc2\xb7  ")
                              + tr("patcher-canvas-mouse.ch-n", "ch {n}", {{"n", juce::String(channel)}})
                        : juce::String();
}
juce::String portWord(PortKind kind, bool out, int index) {
    const TrArgs n{{"n", juce::String(index)}};
    switch (kind) {
        case PortKind::Midi:
            return out ? tr("patcher-canvas-mouse.midi-out-n", "MIDI out {n}", n)
                       : tr("patcher-canvas-mouse.midi-in-n", "MIDI in {n}", n);
        case PortKind::Video:
            return out ? tr("patcher-canvas-mouse.video-out-n", "video out {n}", n)
                       : tr("patcher-canvas-mouse.video-in-n", "video in {n}", n);
        case PortKind::Control:
            return out ? tr("patcher-canvas-mouse.control-out-n", "control out {n}", n)
                       : tr("patcher-canvas-mouse.control-in-n", "control in {n}", n);
        case PortKind::Audio: break;
    }
    return out ? tr("patcher-canvas-mouse.audio-out-n", "audio out {n}", n)
               : tr("patcher-canvas-mouse.audio-in-n", "audio in {n}", n);
}
juce::String nodeIn(const std::string& node, int inlet) {
    return tr("patcher-canvas-mouse.node-in-n", "{node}  in {n}",
              {{"node", juce::String(node)}, {"n", juce::String(inlet + 1)}});
}
juce::String nodeOut(const std::string& node, int outlet) {
    return tr("patcher-canvas-mouse.node-out-n", "{node}  out {n}",
              {{"node", juce::String(node)}, {"n", juce::String(outlet + 1)}});
}
}

juce::String PatcherCanvas::getTooltip() {
    const auto& model = host_.model();
    if (portHover_ && hoverPortControl_) {
        const auto label = hoverPortOut_ ? host_.controlOutletValue(hoverPortNode_, hoverPort_)
                                         : host_.controlInletParam(hoverPortNode_, hoverPort_);
        juce::String t = juce::String(hoverPortNode_) + dash()
                       + portWord(PortKind::Control, hoverPortOut_, hoverPort_ + 1)
                       + "  " + juce::String(label);
        if (hoverPortOut_)
            if (const auto now = host_.controlOutletReading(hoverPortNode_, hoverPort_); !now.empty())
                t += "  =  " + juce::String(now);
        return t;
    }
    if (portHover_) {
        const auto kind = hoverPortMidi_ ? PortKind::Midi : hoverPortVideo_ ? PortKind::Video : PortKind::Audio;
        juce::String t = juce::String(hoverPortNode_) + dash() + portWord(kind, hoverPortOut_, hoverPort_ + 1);
        if (!hoverPortMidi_ && !hoverPortVideo_)
            if (const auto label = portName(hoverPortNode_, hoverPort_, hoverPortOut_); !label.empty())
                t = juce::String(hoverPortNode_) + dash() + juce::String(label) + "  ("
                    + portWord(PortKind::Audio, hoverPortOut_, hoverPort_ + 1) + ")";
        std::string node = hoverPortNode_;
        int port = hoverPort_;
        realPort(hoverPortNode_, hoverPort_, hoverPortOut_, hoverPortMidi_, node, port);
        juce::StringArray wires;
        const auto& cords = hoverPortMidi_ ? model.midiConnections
                          : hoverPortVideo_ ? model.videoConnections : model.connections;
        for (const auto& c : cords) {
            if (hoverPortOut_ && c.src == node && c.srcOutlet == port)
                wires.add(arrowTo() + nodeIn(c.dst, c.dstInlet)
                          + (hoverPortMidi_ ? chanSuffix(c.midiChannel) : juce::String()));
            if (!hoverPortOut_ && c.dst == node && c.dstInlet == port)
                wires.add(arrowFrom() + nodeOut(c.src, c.srcOutlet)
                          + (hoverPortMidi_ ? chanSuffix(c.midiChannel) : juce::String()));
        }
        return t + "\n" + (wires.isEmpty() ? tr("patcher-canvas-mouse.not-connected", "not connected")
                                           : wires.joinIntoString("\n"));
    }
    if (cordHovered_) {
        const auto& c = hoverCord_;
        juce::String t = (c.midi ? tr("patcher-canvas-mouse.midi-cord", "MIDI cord")
                          : c.video ? tr("patcher-canvas-mouse.video-cord", "video cord")
                                    : tr("patcher-canvas-mouse.audio-cord", "audio cord")) + "\n"
                       + nodeOut(c.src, c.srcOutlet) + "  " + arrowTo() + nodeIn(c.dst, c.dstInlet);
        if (c.midi) {
            const int ch = host_.midiCordChannel(c.src, c.srcOutlet, c.dst, c.dstInlet);
            t += "\n" + (ch >= 1 ? tr("patcher-canvas-mouse.channel-n", "channel {n}", {{"n", juce::String(ch)}})
                                 : tr("patcher-canvas-mouse.all-channels-omni", "all channels (omni)"));
        }
        return t;
    }
    return {};
}

}
