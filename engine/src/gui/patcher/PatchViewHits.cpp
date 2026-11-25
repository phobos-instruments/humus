// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/PatchView.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace hum {

namespace {

constexpr int kCurveSteps = 24;
constexpr float kCordReach = 6.0f;

std::array<juce::Point<float>, kCurveSteps + 1> curvePoints(juce::Point<float> a, juce::Point<float> b) {
    const juce::Point<float> c1(a.x, a.y + 30), c2(b.x, b.y - 30);
    std::array<juce::Point<float>, kCurveSteps + 1> pts;
    for (int i = 0; i <= kCurveSteps; ++i) {
        const float t = (float) i / (float) kCurveSteps, u = 1.0f - t;
        pts[(size_t) i] = a * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + b * (t * t * t);
    }
    return pts;
}

}

std::string PatchView::hitNode(juce::Point<int> p) {
    std::string found;
    for (auto& d : displayNodes())
        if (nodeBounds(d.name).contains(p)) found = d.name;
    return found;
}

bool PatchView::hitPort(juce::Point<int> p, std::string& node, int& port,
                            bool& isOutlet, bool& isMidi, bool& isVideo, bool& isControl) {
    isControl = false;
    for (auto& d : displayNodes()) {
        int ins, outs, mIns, mOuts, vIns, vOuts, cIns, cOuts;
        portCounts(d.name, ins, outs);
        midiPortCounts(d.name, mIns, mOuts);
        videoPortCounts(d.name, vIns, vOuts);
        controlPortCounts(d.name, cIns, cOuts);
        for (int i = 0; i < cIns; ++i)
            if (controlInletPos(d.name, i).getDistanceFrom(p) <= kPort / 2.0f + 3.0f) {
                node = d.name; port = i; isOutlet = false; isMidi = false; isVideo = false; isControl = true; return true;
            }
        for (int o = 0; o < cOuts; ++o)
            if (controlOutletPos(d.name, o).getDistanceFrom(p) <= kPort / 2.0f + 3.0f) {
                node = d.name; port = o; isOutlet = true; isMidi = false; isVideo = false; isControl = true; return true;
            }
        if (d.pod) {
            const auto& m = src_.model();
            ins -= pods::strayCount(m, d.name, true);
            outs -= pods::strayCount(m, d.name, false);
            mIns -= pods::strayCount(m, d.name, true, pods::Domain::Midi);
            mOuts -= pods::strayCount(m, d.name, false, pods::Domain::Midi);
        }
        const float tol = kPort / 2.0f + 3.0f;
        isVideo = false;
        for (int i = 0; i < ins; ++i)
            if (inletPos(d.name, i, ins).getDistanceFrom(p) <= tol) {
                node = d.name; port = i; isOutlet = false; isMidi = false; return true;
            }
        for (int o = 0; o < outs; ++o)
            if (outletPos(d.name, o, outs).getDistanceFrom(p) <= tol) {
                node = d.name; port = o; isOutlet = true; isMidi = false; return true;
            }
        for (int i = 0; i < mIns; ++i)
            if (midiInletPos(d.name, i).getDistanceFrom(p) <= tol) {
                node = d.name; port = i; isOutlet = false; isMidi = true; return true;
            }
        for (int o = 0; o < mOuts; ++o)
            if (midiOutletPos(d.name, o).getDistanceFrom(p) <= tol) {
                node = d.name; port = o; isOutlet = true; isMidi = true; return true;
            }
        for (int i = 0; i < vIns; ++i)
            if (videoInletPos(d.name, i).getDistanceFrom(p) <= tol) {
                node = d.name; port = i; isOutlet = false; isMidi = false; isVideo = true; return true;
            }
        for (int o = 0; o < vOuts; ++o)
            if (videoOutletPos(d.name, o).getDistanceFrom(p) <= tol) {
                node = d.name; port = o; isOutlet = true; isMidi = false; isVideo = true; return true;
            }
    }
    return false;
}

std::vector<PortSpot> PatchView::portSpots(const std::string& name) {
    std::vector<PortSpot> spots;
    auto add = [&](PortKind kind, bool outlet, int index, std::string label, juce::Point<int> at) {
        spots.push_back({kind, outlet, index, std::move(label), at});
    };
    int ins, outs;
    portCounts(name, ins, outs);
    for (int i = 0; i < ins; ++i) add(PortKind::Audio, false, i, portName(name, i, false), inletPos(name, i, ins));
    for (int o = 0; o < outs; ++o) add(PortKind::Audio, true, o, portName(name, o, true), outletPos(name, o, outs));
    int mIns, mOuts;
    midiPortCounts(name, mIns, mOuts);
    for (int i = 0; i < mIns; ++i) add(PortKind::Midi, false, i, {}, midiInletPos(name, i));
    for (int o = 0; o < mOuts; ++o) add(PortKind::Midi, true, o, {}, midiOutletPos(name, o));
    int vIns, vOuts;
    videoPortCounts(name, vIns, vOuts);
    for (int i = 0; i < vIns; ++i) add(PortKind::Video, false, i, {}, videoInletPos(name, i));
    for (int o = 0; o < vOuts; ++o) add(PortKind::Video, true, o, {}, videoOutletPos(name, o));
    int cIns, cOuts;
    controlPortCounts(name, cIns, cOuts);
    for (int i = 0; i < cIns; ++i)
        add(PortKind::Control, false, i, src_.controlInletParam(name, i), controlInletPos(name, i));
    for (int o = 0; o < cOuts; ++o)
        add(PortKind::Control, true, o, src_.controlOutletValue(name, o), controlOutletPos(name, o));
    return spots;
}

juce::Point<int> PatchView::portPos(const PortRef& at) {
    if (at.control) return at.outlet ? controlOutletPos(at.node, at.port) : controlInletPos(at.node, at.port);
    if (at.midi) return at.outlet ? midiOutletPos(at.node, at.port) : midiInletPos(at.node, at.port);
    if (at.video) return at.outlet ? videoOutletPos(at.node, at.port) : videoInletPos(at.node, at.port);
    int ins = 0, outs = 0;
    portCounts(at.node, ins, outs);
    return at.outlet ? outletPos(at.node, at.port, outs) : inletPos(at.node, at.port, ins);
}

std::vector<PatchView::CordLine> PatchView::visibleCords() {
    std::vector<CordLine> out;
    auto scan = [&](const std::vector<ConnectionModel>& list, pods::Domain dom) {
        const bool midi = dom == pods::Domain::Midi, video = dom == pods::Domain::Video;
        for (const auto& c : list) {
            std::string sn, dn;
            int sp = 0, dp = 0;
            if (!mapEndpoint(c.src, c.srcOutlet, false, dom, sn, sp)
                || !mapEndpoint(c.dst, c.dstInlet, true, dom, dn, dp) || sn == dn)
                continue;
            int outs = 0, ins = 0, unused = 0;
            if (midi) { midiPortCounts(sn, unused, outs); midiPortCounts(dn, ins, unused); }
            else if (video) { videoPortCounts(sn, unused, outs); videoPortCounts(dn, ins, unused); }
            else { portCounts(sn, unused, outs); portCounts(dn, ins, unused); }
            if (outs == 0 || ins == 0) continue;
            const auto a = midi ? midiOutletPos(sn, sp) : video ? videoOutletPos(sn, sp) : outletPos(sn, sp, outs);
            const auto b = midi ? midiInletPos(dn, dp) : video ? videoInletPos(dn, dp) : inletPos(dn, dp, ins);
            out.push_back({a.toFloat(), b.toFloat(), Edge{c.src, c.srcOutlet, c.dst, c.dstInlet, midi, video}});
        }
    };
    scan(src_.model().connections, pods::Domain::Audio);
    scan(src_.model().midiConnections, pods::Domain::Midi);
    scan(src_.model().videoConnections, pods::Domain::Video);
    for (const auto& c : src_.controlCordsInScope(scope_))
        out.push_back({controlOutletPos(c.src, c.srcOutlet).toFloat(), controlInletPos(c.dst, c.dstInlet).toFloat(),
                       Edge{c.src, c.srcOutlet, c.dst, c.dstInlet, false, false, true}});
    return out;
}

bool PatchView::hitCord(juce::Point<int> p, Edge& out) const {
    auto* self = const_cast<PatchView*>(this);
    if (!self->hitNode(p).empty()) return false;
    const auto fp = p.toFloat();
    float best = kCordReach;
    bool found = false;
    for (const auto& line : self->visibleCords()) {
        const auto pts = curvePoints(line.a, line.b);
        for (size_t i = 1; i < pts.size(); ++i) {
            juce::Point<float> onLine;
            const float d = juce::Line<float>(pts[i - 1], pts[i]).getDistanceFromPoint(fp, onLine);
            if (d <= kCordReach && (!found || d < best)) {
                best = d;
                out = line.cord;
                found = true;
            }
        }
    }
    return found;
}

std::vector<PatchView::Edge> PatchView::cordsCrossing(juce::Rectangle<int> band) {
    std::vector<Edge> out;
    const auto r = band.toFloat();
    for (const auto& line : visibleCords()) {
        const auto pts = curvePoints(line.a, line.b);
        for (size_t i = 1; i < pts.size(); ++i)
            if (r.intersects(juce::Line<float>(pts[i - 1], pts[i]))) {
                out.push_back(line.cord);
                break;
            }
    }
    return out;
}

bool PatchView::isCordSelected(const Edge& e) const {
    return std::find(selectedCords_.begin(), selectedCords_.end(), e) != selectedCords_.end();
}

}
