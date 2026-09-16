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
    if (hitPort(p, node, port, isOutlet, isMidi, isVideo, isControl) && isOutlet) {
        drag_ = Drag::Cord; dragNode_ = node; cordOutlet_ = port;
        cordMidi_ = isMidi; cordVideo_ = isVideo; cordControl_ = isControl; cordEnd_ = p;
        return;
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
        std::string node; int port; bool isOutlet, isMidi, isVideo, isControl;
        cordTarget_ = hitPort(cordEnd_, node, port, isOutlet, isMidi, isVideo, isControl)
                      && !isOutlet && node != dragNode_
                      && isMidi == cordMidi_ && isVideo == cordVideo_ && isControl == cordControl_;
        if (cordTarget_) { cordTargetNode_ = node; cordTargetPort_ = port; }
        refresh();
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
            && !isOutlet && node != dragNode_
            && isMidi == cordMidi_ && isVideo == cordVideo_ && isControl == cordControl_) {
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
                std::string rs, rd;
                int rsp, rdp;
                realPort(dragNode_, cordOutlet_, true, false, rs, rsp);
                realPort(node, port, false, false, rd, rdp);
                bool stereo = !e.mods.isAltDown()
                           && (cordOutlet_ % 2) == 0 && (port % 2) == 0;
                std::string rs2, rd2;
                int rsp2 = -1, rdp2 = -1;
                if (stereo) {
                    int sIns = 0, sOuts = 0, dIns = 0, dOuts = 0;
                    portCounts(dragNode_, sIns, sOuts);
                    portCounts(node, dIns, dOuts);
                    stereo = cordOutlet_ + 1 < sOuts && port + 1 < dIns;
                }
                if (stereo) {
                    realPort(dragNode_, cordOutlet_ + 1, true, false, rs2, rsp2);
                    realPort(node, port + 1, false, false, rd2, rdp2);
                    for (auto& c : host_.model().connections)
                        if (c.dst == rd2 && c.dstInlet == rdp2) { stereo = false; break; }
                }
                if (stereo) {
                    host_.beginTransaction();
                    host_.connect(rs, rsp, rd, rdp);
                    host_.connect(rs2, rsp2, rd2, rdp2);
                    host_.endTransaction();
                } else {
                    host_.connect(rs, rsp, rd, rdp);
                }
            }
        }
    } else if (drag_ == Drag::Select) {
        primary_ = selection_.size() == 1 ? *selection_.begin() : "";
        notifySelection();
    } else if (drag_ == Drag::Move) {
        commitDragHints();
    }
    if (drag_ == Drag::Pan) setMouseCursor(juce::MouseCursor::NormalCursor);
    drag_ = Drag::None;
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

    setMouseCursor((ph || h) ? juce::MouseCursor::PointingHandCursor
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
    return channel >= 1 ? juce::String::fromUTF8("  \xc2\xb7  ch ") + juce::String(channel)
                        : juce::String();
}
}

juce::String PatcherCanvas::getTooltip() {
    const auto& model = host_.model();
    if (portHover_ && hoverPortControl_) {
        const auto label = hoverPortOut_ ? host_.controlOutletValue(hoverPortNode_, hoverPort_)
                                         : host_.controlInletParam(hoverPortNode_, hoverPort_);
        return juce::String(hoverPortNode_) + dash() + (hoverPortOut_ ? "control out " : "control in ")
             + juce::String(hoverPort_ + 1) + "  " + juce::String(label);
    }
    if (portHover_) {
        juce::String t = juce::String(hoverPortNode_) + dash()
                       + (hoverPortMidi_ ? "MIDI " : hoverPortVideo_ ? "video " : "audio ")
                       + (hoverPortOut_ ? "out " : "in ") + juce::String(hoverPort_ + 1);
        std::string node = hoverPortNode_;
        int port = hoverPort_;
        realPort(hoverPortNode_, hoverPort_, hoverPortOut_, hoverPortMidi_, node, port);
        juce::StringArray wires;
        const auto& cords = hoverPortMidi_ ? model.midiConnections
                          : hoverPortVideo_ ? model.videoConnections : model.connections;
        for (const auto& c : cords) {
            if (hoverPortOut_ && c.src == node && c.srcOutlet == port)
                wires.add(arrowTo() + juce::String(c.dst) + "  in " + juce::String(c.dstInlet + 1)
                          + (hoverPortMidi_ ? chanSuffix(c.midiChannel) : juce::String()));
            if (!hoverPortOut_ && c.dst == node && c.dstInlet == port)
                wires.add(arrowFrom() + juce::String(c.src) + "  out " + juce::String(c.srcOutlet + 1)
                          + (hoverPortMidi_ ? chanSuffix(c.midiChannel) : juce::String()));
        }
        return t + "\n" + (wires.isEmpty() ? juce::String("not connected")
                                           : wires.joinIntoString("\n"));
    }
    if (cordHovered_) {
        const auto& c = hoverCord_;
        juce::String t = juce::String(c.midi ? tr("patcher-canvas-mouse.midi-cord", "MIDI cord") : c.video ? "video cord" : "audio cord") + "\n"
                       + juce::String(c.src) + "  out " + juce::String(c.srcOutlet + 1)
                       + "  " + arrowTo()
                       + juce::String(c.dst) + "  in " + juce::String(c.dstInlet + 1);
        if (c.midi) {
            const int ch = host_.midiCordChannel(c.src, c.srcOutlet, c.dst, c.dstInlet);
            t += ch >= 1 ? "\nchannel " + juce::String(ch) : juce::String("\nall channels (omni)");
        }
        return t;
    }
    return {};
}

}
