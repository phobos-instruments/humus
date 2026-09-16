// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <algorithm>

#include "gui/host/EngineHost.h"
#include "io/ModRouteBuild.h"
#include "core/params/ParamSchema.h"

#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"
#include "core/packs/Roles.h"
#include "hum/Registry.h"

namespace hum {

void EngineHost::connect(const std::string& src, int outlet, const std::string& dst, int inlet) {
    if (src == dst) return;
    if (cordWouldStray(src, outlet, dst, inlet, pods::Domain::Audio)) return;
    pushUndo();
    model_.connections.push_back({src, outlet, dst, inlet});
    requestRebuild();
}

void EngineHost::disconnectInto(const std::string& dst, int inlet) {
    pushUndo();
    auto& cn = model_.connections;
    cn.erase(std::remove_if(cn.begin(), cn.end(),
                            [&](const ConnectionModel& c) { return c.dst == dst && c.dstInlet == inlet; }), cn.end());
    requestRebuild();
}

void EngineHost::removeConnection(const std::string& src, int outlet,
                                  const std::string& dst, int inlet) {
    pushUndo();
    auto& cn = model_.connections;
    cn.erase(std::remove_if(cn.begin(), cn.end(), [&](const ConnectionModel& c) {
        return c.src == src && c.srcOutlet == outlet && c.dst == dst && c.dstInlet == inlet;
    }), cn.end());
    requestRebuild();
}

bool EngineHost::isConnected(const std::string& src, int outlet,
                             const std::string& dst, int inlet) const {
    for (auto& c : model_.connections)
        if (c.src == src && c.srcOutlet == outlet && c.dst == dst && c.dstInlet == inlet)
            return true;
    return false;
}

void EngineHost::applyBypass(const std::string& name, bool on) {
    auto* cm = model_.byName(name);
    if (cm == nullptr) return;
    bool found = false;
    for (auto& p : cm->properties)
        if (p.name == kBypassParam) {
            p.value = on ? 1.0 : 0.0;
            p.userEdited = true;
            found = true;
        }
    if (!found) {
        Parameter p;
        p.index = (int) cm->properties.size();
        p.name = kBypassParam;
        p.type = "bool";
        p.value = on ? 1.0 : 0.0;
        p.userEdited = true;
        cm->properties.push_back(p);
    }
    if (graph_) graph_->setNodeBypass(graph_->indexOf(name), on);
}

void EngineHost::applyTrackMute(const std::string& name, bool on) {
    auto* cm = model_.byName(name);
    if (cm == nullptr) return;
    bool found = false;
    for (auto& p : cm->properties)
        if (p.name == kTrackMuteParam) {
            p.value = on ? 1.0 : 0.0;
            p.userEdited = true;
            found = true;
        }
    if (!found) {
        Parameter p;
        p.index = (int) cm->properties.size();
        p.name = kTrackMuteParam;
        p.type = "bool";
        p.value = on ? 1.0 : 0.0;
        p.userEdited = true;
        cm->properties.push_back(p);
    }
    if (graph_) graph_->setNodeTrackMuted(graph_->indexOf(name), on);
}

void EngineHost::setTrackMuted(const std::string& name, bool on) {
    const auto* cm = model_.byName(name);
    if (cm == nullptr || modelTrackMuted(*cm) == on) return;
    pushUndo();
    applyTrackMute(name, on);
    dirty_ = true;
    ++changeStamp_;
}

bool EngineHost::trackMuted(const std::string& name) const {
    const auto* cm = model_.byName(name);
    return cm != nullptr && modelTrackMuted(*cm);
}

int EngineHost::connectToMaster(const std::string& node) {
    const auto master = masterOutputName();
    if (master.empty()) return 0;
    const int n = std::max(1, std::min(outletsOf(node), inletsOf(master)));
    for (int ch = 0; ch < n; ++ch) connect(node, ch, master, ch);
    return n;
}

void EngineHost::setSoloed(const std::string& name, bool on) {
    if (on) solo_.insert(name);
    else    solo_.erase(name);
    applySolo();
    ++changeStamp_;
}

void EngineHost::applySolo() {
    if (graph_ == nullptr) return;
    for (const auto& cm : model_.organisms) {
        const int idx = graph_->indexOf(cm.name);
        if (idx < 0) continue;
        const bool doc = modelBypassed(cm);
        if (solo_.empty()) { graph_->setNodeBypass(idx, doc); continue; }
        const bool arranges = std::find(soloable_.begin(), soloable_.end(), cm.name)
                              != soloable_.end();
        graph_->setNodeBypass(idx, doc || (arranges && solo_.count(cm.name) == 0));
    }
}

void EngineHost::setBypass(const std::string& name, bool on) {
    const auto* cm = model_.byName(name);
    if (cm == nullptr || modelBypassed(*cm) == on) return;
    pushUndo();
    applyBypass(name, on);
    dirty_ = true;
    ++changeStamp_;
}

bool EngineHost::bypassed(const std::string& name) const {
    const auto* cm = model_.byName(name);
    return cm != nullptr && modelBypassed(*cm);
}

std::string EngineHost::missingClassNote(const std::string& name) const {
    const auto* cm = model_.byName(name);
    if (cm == nullptr || isPluginKind(cm->kind)) return {};
    auto& pr = PackRegistry::instance();
    if (const auto* p = pr.packOf(cm->classRaw); p != nullptr && !p->enabled)
        return "The " + p->manifest.name + " pack is disabled";
    if (!Registry::instance().isKnown(cm->classRaw))
        return "No installed pack provides this organism";
    return {};
}

static void planBridge(const std::vector<ConnectionModel>& cords, const std::string& name,
                       int n, std::vector<ConnectionModel>& bridges,
                       std::vector<ConnectionModel>& old) {
    for (int k = 0; k < n; ++k) {
        std::vector<std::pair<std::string, int>> srcs, dsts;
        for (auto& c : cords) {
            if (c.dst == name && c.dstInlet  == k) srcs.push_back({c.src, c.srcOutlet});
            if (c.src == name && c.srcOutlet == k) dsts.push_back({c.dst, c.dstInlet});
        }
        for (auto& s : srcs) for (auto& d : dsts) bridges.push_back({s.first, s.second, d.first, d.second});
    }
    for (auto& c : cords) if (c.src == name || c.dst == name) old.push_back(c);
}

void EngineHost::disconnectOrganism(const std::string& name) {
    if (!model_.byName(name)) return;
    beginTransaction();
    std::vector<ConnectionModel> bridges, old;
    planBridge(model_.connections, name,
               juce::jmax(inletsOf(name), outletsOf(name)), bridges, old);
    for (auto& c : old)     removeConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& b : bridges) connect(b.src, b.srcOutlet, b.dst, b.dstInlet);

    bridges.clear(); old.clear();
    planBridge(model_.midiConnections, name,
               juce::jmax(midiInletsOf(name), midiOutletsOf(name)), bridges, old);
    for (auto& c : old)     removeMidiConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& b : bridges) connectMidi(b.src, b.srcOutlet, b.dst, b.dstInlet);

    bridges.clear(); old.clear();
    planBridge(model_.videoConnections, name,
               juce::jmax(videoInletsOf(name), videoOutletsOf(name)), bridges, old);
    for (auto& c : old)     removeVideoConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& b : bridges) connectVideo(b.src, b.srcOutlet, b.dst, b.dstInlet);
    endTransaction();
}

}
