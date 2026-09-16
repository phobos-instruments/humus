// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include "gui/host/CordSplice.h"
#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/graph/CordReclaim.h"
#include "core/packs/PackManifest.h"
#include "core/plugins/PluginHost.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PodModel.h"
#include "core/packs/StripFamily.h"
#include "core/params/Randomize.h"
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "core/params/ParamSchema.h"
#include "core/packs/Roles.h"
#include "gui/properties/PresetLibrary.h"
#include "hum/Registry.h"

namespace hum {

PodClip EngineHost::capturePod(const std::string& pod) const {
    PodClip k;
    k.leaf = pods::leafOf(pod);
    const std::string pre = pod + "/";
    auto under = [&](const std::string& n) { return n.rfind(pre, 0) == 0; };
    auto rel = [&](const std::string& n) { return n.substr(pre.size()); };
    for (const auto& c : model_.organisms)
        if (under(c.name)) {
            auto m = c;
            m.name = rel(c.name);
            k.nodes.push_back(std::move(m));
        }
    auto capture = [&](const std::vector<ConnectionModel>& from,
                       std::vector<ConnectionModel>& to) {
        for (const auto& c : from)
            if (under(c.src) && under(c.dst)) {
                auto c2 = c;
                c2.src = rel(c.src);
                c2.dst = rel(c.dst);
                to.push_back(std::move(c2));
            }
    };
    capture(model_.connections, k.cords);
    capture(model_.midiConnections, k.midiCords);
    capture(model_.videoConnections, k.videoCords);
    for (const auto& [n, p] : positions_)
        if (under(n)) k.layout.emplace_back(rel(n), p);
    if (auto it = positions_.find(pod); it != positions_.end()) k.boxPos = it->second;
    return k;
}

std::string EngineHost::pastePod(const PodClip& clip, juce::Point<int> at,
                                 const std::string& scope) {
    if (clip.nodes.empty()) return {};
    pushUndo();
    const std::string want = (scope.empty() ? std::string() : scope + "/") + clip.leaf;
    std::string pod = want;
    for (int n = 2; model_.byName(pod) || pods::isPod(model_, pod); ++n)
        pod = want + "_" + std::to_string(n);
    for (const auto& m : clip.nodes) {
        auto c = m;
        c.name = pod + "/" + m.name;
        model_.organisms.push_back(std::move(c));
    }
    for (const auto& cn : clip.cords) {
        auto c = cn;
        c.src = pod + "/" + cn.src;
        c.dst = pod + "/" + cn.dst;
        model_.connections.push_back(std::move(c));
    }
    for (const auto& cn : clip.midiCords) {
        auto c = cn;
        c.src = pod + "/" + cn.src;
        c.dst = pod + "/" + cn.dst;
        model_.midiConnections.push_back(std::move(c));
    }
    for (const auto& cn : clip.videoCords) {
        auto c = cn;
        c.src = pod + "/" + cn.src;
        c.dst = pod + "/" + cn.dst;
        model_.videoConnections.push_back(std::move(c));
    }
    for (const auto& [n, p] : clip.layout) positions_[pod + "/" + n] = p;
    positions_[pod] = at;
    requestRebuild();
    return pod;
}

void EngineHost::disconnectPod(const std::string& pod) {
    const auto ins = pods::inletRefs(model_, pod);
    const auto outs = pods::outletRefs(model_, pod);
    if (ins.empty() && outs.empty()) return;
    beginTransaction();
    const std::string pre = pod + "/";
    auto external = [&](const std::string& n) { return n.rfind(pre, 0) != 0; };
    const int n = (int) std::max(ins.size(), outs.size());
    std::vector<ConnectionModel> bridges, old;
    for (int k = 0; k < n; ++k) {
        std::vector<std::pair<std::string, int>> srcs, dsts;
        for (auto& c : model_.connections) {
            if (k < (int) ins.size() && c.dst == ins[(size_t) k].node
                && c.dstInlet == ins[(size_t) k].chan && external(c.src))
                srcs.push_back({c.src, c.srcOutlet});
            if (k < (int) outs.size() && c.src == outs[(size_t) k].node
                && c.srcOutlet == outs[(size_t) k].chan && external(c.dst))
                dsts.push_back({c.dst, c.dstInlet});
        }
        for (auto& s : srcs)
            for (auto& d : dsts) bridges.push_back({s.first, s.second, d.first, d.second});
    }
    auto isBoundary = [](const std::vector<pods::PortRef>& refs, const std::string& node) {
        for (const auto& r : refs) if (r.node == node) return true;
        return false;
    };
    for (auto& c : model_.connections) {
        const bool in = isBoundary(ins, c.dst) && external(c.src);
        const bool out = isBoundary(outs, c.src) && external(c.dst);
        if (in || out) old.push_back(c);
    }
    for (auto& c : old) removeConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& b : bridges) connect(b.src, b.srcOutlet, b.dst, b.dstInlet);
    endTransaction();
}

std::string EngineHost::insertBeforePod(const std::string& pod, const std::string& newClass) {
    const auto ins = pods::declaredRefs(model_, pod, true);
    if (ins.empty()) return {};
    beginTransaction();
    const auto pos = position(pod);
    const auto nn = addOrganism(newClass, {pos.x, juce::jmax(0, pos.y - 70)},
                                   pods::parentOf(pod));
    const auto io = classIO(newClass);
    const std::string pre = pod + "/";
    auto external = [&](const std::string& n) { return n.rfind(pre, 0) != 0; };
    struct Feeder { std::string src; int srcOutlet; int chan; };
    std::vector<Feeder> feeders;
    for (int k = 0; k < (int) ins.size(); ++k)
        for (auto& c : model_.connections)
            if (c.dst == ins[(size_t) k].node && c.dstInlet == ins[(size_t) k].chan
                && external(c.src))
                feeders.push_back({c.src, c.srcOutlet, k});
    for (auto& f : feeders) {
        const auto& r = ins[(size_t) f.chan];
        removeConnection(f.src, f.srcOutlet, r.node, r.chan);
        if (io.first > 0) connect(f.src, f.srcOutlet, nn, std::min(f.chan, io.first - 1));
    }
    for (int k = 0; k < std::min(io.second, (int) ins.size()); ++k)
        connect(nn, k, ins[(size_t) k].node, ins[(size_t) k].chan);
    endTransaction();
    return nn;
}

std::string EngineHost::insertAfterPod(const std::string& pod, const std::string& newClass) {
    const auto outs = pods::declaredRefs(model_, pod, false);
    if (outs.empty()) return {};
    beginTransaction();
    const auto pos = position(pod);
    const auto nn = addOrganism(newClass, {pos.x, pos.y + 70}, pods::parentOf(pod));
    const auto io = classIO(newClass);
    const std::string pre = pod + "/";
    auto external = [&](const std::string& n) { return n.rfind(pre, 0) != 0; };
    struct Consumer { std::string dst; int dstInlet; int chan; };
    std::vector<Consumer> consumers;
    for (int k = 0; k < (int) outs.size(); ++k)
        for (auto& c : model_.connections)
            if (c.src == outs[(size_t) k].node && c.srcOutlet == outs[(size_t) k].chan
                && external(c.dst))
                consumers.push_back({c.dst, c.dstInlet, k});
    for (auto& u : consumers) {
        const auto& r = outs[(size_t) u.chan];
        removeConnection(r.node, r.chan, u.dst, u.dstInlet);
        if (io.second > 0) connect(nn, std::min(u.chan, io.second - 1), u.dst, u.dstInlet);
    }
    for (int k = 0; k < std::min(io.first, (int) outs.size()); ++k)
        connect(outs[(size_t) k].node, outs[(size_t) k].chan, nn, k);
    endTransaction();
    return nn;
}

}
