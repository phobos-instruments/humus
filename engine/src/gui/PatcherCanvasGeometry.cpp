#include "gui/PatcherCanvas.h"

#include <algorithm>

#include "core/Categories.h"
#include "core/PodModel.h"
#include "gui/AppSettings.h"

namespace hum {

static bool hidden(const OrganismModel& c) {
    return c.internal || isHiddenOrganism(c.displayClass);
}

std::vector<PatcherCanvas::DisplayNode> PatcherCanvas::displayNodes() const {
    std::vector<DisplayNode> out;
    for (auto& c : host_.model().organisms)
        if (!hidden(c) && pods::inScope(c.name, scope_)) out.push_back({c.name, false});
    for (auto& p : pods::podsIn(host_.model(), scope_)) out.push_back({p, true});
    return out;
}

bool PatcherCanvas::isPodBox(const std::string& name) const {
    return pods::parentOf(name) == scope_ && pods::isPod(host_.model(), name);
}

void PatcherCanvas::portCounts(const std::string& name, int& ins, int& outs) const {
    if (isPodBox(name)) {
        ins = (int) pods::inletRefs(host_.model(), name).size();
        outs = (int) pods::outletRefs(host_.model(), name).size();
    } else {
        auto& h = const_cast<EngineHost&>(host_);
        ins = h.inletsOf(name);
        outs = h.outletsOf(name);
        if (!scope_.empty() && pods::parentOf(name) == scope_)
            if (const auto* cm = host_.model().byName(name)) {
                if (pods::isInletClass(cm->displayClass)) ins = 0;
                else if (pods::isOutletClass(cm->displayClass)) outs = 0;
            }
    }
}

void PatcherCanvas::midiPortCounts(const std::string& name, int& ins, int& outs) const {
    if (isPodBox(name)) {
        ins = (int) pods::midiInletRefs(host_.model(), name).size();
        outs = (int) pods::midiOutletRefs(host_.model(), name).size();
        return;
    }
    auto& h = const_cast<EngineHost&>(host_);
    ins = h.midiInletsOf(name);
    outs = h.midiOutletsOf(name);
    if (!scope_.empty() && pods::parentOf(name) == scope_)
        if (const auto* cm = host_.model().byName(name)) {
            if (pods::isInletClass(cm->displayClass)) ins = 0;
            else if (pods::isOutletClass(cm->displayClass)) outs = 0;
        }
}

void PatcherCanvas::videoPortCounts(const std::string& name, int& ins, int& outs) const {
    if (isPodBox(name)) {
        ins = (int) pods::videoPortRefs(host_.model(), name, true).size();
        outs = (int) pods::videoPortRefs(host_.model(), name, false).size();
        return;
    }
    auto& h = const_cast<EngineHost&>(host_);
    ins = h.videoInletsOf(name);
    outs = h.videoOutletsOf(name);
    if (!scope_.empty() && pods::parentOf(name) == scope_)
        if (const auto* cm = host_.model().byName(name)) {
            if (pods::isInletClass(cm->displayClass)) ins = 0;
            else if (pods::isOutletClass(cm->displayClass)) outs = 0;
        }
}

bool PatcherCanvas::mapEndpoint(const std::string& node, int port, bool isDstSide,
                                pods::Domain dom, std::string& dispNode, int& dispPort) const {
    const auto s = pods::surfaceOf(host_.model(), node, port, isDstSide, scope_, dom);
    if (s.kind == pods::Surface::None) return false;
    if (s.kind == pods::Surface::Self) {
        if (auto* m = host_.model().byName(node); m && hidden(*m)) return false;
        dispNode = node;
        dispPort = port;
        return true;
    }
    dispNode = s.box;
    dispPort = s.pin;
    return true;
}

void PatcherCanvas::realPort(const std::string& dispNode, int port, bool isOutlet,
                             pods::Domain dom, std::string& realNode, int& realPort) const {
    if (isPodBox(dispNode)) {
        const auto list =
            dom == pods::Domain::Midi
                ? (isOutlet ? pods::midiOutletRefs(host_.model(), dispNode)
                            : pods::midiInletRefs(host_.model(), dispNode))
            : dom == pods::Domain::Video
                ? pods::videoPortRefs(host_.model(), dispNode, !isOutlet)
                : (isOutlet ? pods::outletRefs(host_.model(), dispNode)
                            : pods::inletRefs(host_.model(), dispNode));
        if (port >= 0 && port < (int) list.size()) {
            realNode = list[(size_t) port].node;
            realPort = list[(size_t) port].chan;
            return;
        }
    }
    realNode = dispNode;
    realPort = port;
}

int PatcherCanvas::nodeWidth(const std::string& name) {
    int ins, outs, mIns, mOuts, vIns, vOuts;
    portCounts(name, ins, outs);
    midiPortCounts(name, mIns, mOuts);
    videoPortCounts(name, vIns, vOuts);
    const int nA = std::max(ins, outs);
    const int nM = std::max(mIns, mOuts);
    const int nV = std::max(vIns, vOuts);
    int need = kPortPad * 2 + kPort + std::max(0, nA - 1) * (kPort + kPortGap);
    if (nV > 0) need += (nA > 0 ? kMidiGap : 0) + nV * (kPort + kPortGap);
    if (nM > 0) need += kMidiGap + nM * (kPort + kPortGap);
    return std::max(kW, need);
}

juce::Rectangle<int> PatcherCanvas::nodeBounds(const std::string& name) {
    auto p = host_.position(name);
    return {p.x, p.y, nodeWidth(name), kH};
}

juce::Point<int> PatcherCanvas::inletPos(const std::string& name, int inlet, int count) {
    juce::ignoreUnused(count);
    auto b = nodeBounds(name);
    return {b.getX() + kPortPad + kPort / 2 + inlet * (kPort + kPortGap), b.getY()};
}
juce::Point<int> PatcherCanvas::outletPos(const std::string& name, int outlet, int count) {
    juce::ignoreUnused(count);
    auto b = nodeBounds(name);
    return {b.getX() + kPortPad + kPort / 2 + outlet * (kPort + kPortGap), b.getBottom()};
}

juce::Point<int> PatcherCanvas::midiInletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int mIns, mOuts;
    midiPortCounts(name, mIns, mOuts);
    return {b.getRight() - kPortPad - kPort / 2 - (mIns - 1 - port) * (kPort + kPortGap),
            b.getY()};
}
juce::Point<int> PatcherCanvas::midiOutletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int mIns, mOuts;
    midiPortCounts(name, mIns, mOuts);
    return {b.getRight() - kPortPad - kPort / 2 - (mOuts - 1 - port) * (kPort + kPortGap),
            b.getBottom()};
}

juce::Point<int> PatcherCanvas::videoInletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int ins, outs;
    portCounts(name, ins, outs);
    const int nA = std::max(ins, outs);
    const int x = kPortPad + kPort / 2 + (nA > 0 ? nA * (kPort + kPortGap) + kMidiGap : 0)
                  + port * (kPort + kPortGap);
    return {b.getX() + x, b.getY()};
}
juce::Point<int> PatcherCanvas::videoOutletPos(const std::string& name, int port) {
    auto b = nodeBounds(name);
    int ins, outs;
    portCounts(name, ins, outs);
    const int nA = std::max(ins, outs);
    const int x = kPortPad + kPort / 2 + (nA > 0 ? nA * (kPort + kPortGap) + kMidiGap : 0)
                  + port * (kPort + kPortGap);
    return {b.getX() + x, b.getBottom()};
}

std::string PatcherCanvas::hitNode(juce::Point<int> p) {
    std::string found;
    for (auto& d : displayNodes())
        if (nodeBounds(d.name).contains(p)) found = d.name;
    return found;
}

bool PatcherCanvas::hitCord(juce::Point<int> p, Edge& out) const {
    const juce::Point<float> fp = p.toFloat();
    float best = 1e9f;
    auto* self = const_cast<PatcherCanvas*>(this);
    auto scan = [&](const std::vector<ConnectionModel>& list, bool midi) {
        for (auto& c : list) {
            juce::Point<float> a, b;
            std::string sn, dn;
            int sp, dp;
            if (!mapEndpoint(c.src, c.srcOutlet, false, midi, sn, sp)
                || !mapEndpoint(c.dst, c.dstInlet, true, midi, dn, dp) || sn == dn)
                continue;
            int so, si, tmp;
            if (midi) {
                midiPortCounts(sn, tmp, so);
                midiPortCounts(dn, si, tmp);
                if (so == 0 || si == 0) continue;
                a = self->midiOutletPos(sn, sp).toFloat();
                b = self->midiInletPos(dn, dp).toFloat();
            } else {
                portCounts(sn, tmp, so);
                portCounts(dn, si, tmp);
                if (so == 0 || si == 0) continue;
                a = self->outletPos(sn, sp, so).toFloat();
                b = self->inletPos(dn, dp, si).toFloat();
            }
            juce::Point<float> c1(a.x, a.y + 30), c2(b.x, b.y - 30);
            for (int i = 0; i <= 24; ++i) {
                float t = i / 24.0f, u = 1.0f - t;
                auto pt = a * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + b * (t * t * t);
                float d = pt.getDistanceFrom(fp);
                if (d < best) { best = d; out = {c.src, c.srcOutlet, c.dst, c.dstInlet, midi}; }
            }
        }
    };
    scan(host_.model().connections, false);
    scan(host_.model().midiConnections, true);
    for (auto& c : host_.model().videoConnections) {
        std::string sn, dn;
        int sp, dp;
        if (!mapEndpoint(c.src, c.srcOutlet, false, pods::Domain::Video, sn, sp)
            || !mapEndpoint(c.dst, c.dstInlet, true, pods::Domain::Video, dn, dp) || sn == dn)
            continue;
        int vo, vi, tmp;
        videoPortCounts(sn, tmp, vo);
        videoPortCounts(dn, vi, tmp);
        if (vo == 0 || vi == 0) continue;
        const auto a = self->videoOutletPos(sn, sp).toFloat();
        const auto b = self->videoInletPos(dn, dp).toFloat();
        juce::Point<float> c1(a.x, a.y + 30), c2(b.x, b.y - 30);
        for (int i = 0; i <= 24; ++i) {
            float t = i / 24.0f, u = 1.0f - t;
            auto pt = a * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + b * (t * t * t);
            float d = pt.getDistanceFrom(fp);
            if (d < best) { best = d; out = {c.src, c.srcOutlet, c.dst, c.dstInlet, false, true}; }
        }
    }
    return best <= 6.0f;
}

bool PatcherCanvas::hitPort(juce::Point<int> p, std::string& node, int& port,
                            bool& isOutlet, bool& isMidi, bool& isVideo) {
    for (auto& d : displayNodes()) {
        int ins, outs, mIns, mOuts, vIns, vOuts;
        portCounts(d.name, ins, outs);
        midiPortCounts(d.name, mIns, mOuts);
        videoPortCounts(d.name, vIns, vOuts);
        if (d.pod) {
            const auto& m = host_.model();
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

}
