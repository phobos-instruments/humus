// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
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

std::string EngineHost::createPod(int ins, int outs, juce::Point<int> at,
                                  const std::string& scope, bool stereoPairs,
                                  int midiIns, int midiOuts) {
    ins = juce::jlimit(0, 16, ins);
    midiIns = juce::jlimit(0, 8, midiIns);
    midiOuts = juce::jlimit(0, 8, midiOuts);
    outs = juce::jlimit(1, 16, outs);
    beginTransaction();
    const std::string base = (scope.empty() ? std::string() : scope + "/") + "Pod_";
    std::string pod;
    for (int n = 1;; ++n) {
        pod = base + std::to_string(n);
        if (!model_.byName(pod) && !pods::isPod(model_, pod)) break;
    }
    auto place = [&](int channels, bool inletSide, int y) {
        int x = 40;
        for (; channels > 0; x += 160) {
            const bool st = stereoPairs && channels >= 2;
            addOrganism(inletSide ? (st ? pods::kInletStereoClass : pods::kInletClass)
                                     : (st ? pods::kOutletStereoClass : pods::kOutletClass),
                           {x, y}, pod);
            channels -= st ? 2 : 1;
        }
        return x;
    };
    const int inX = place(ins, true, 40);
    const int outX = place(outs, false, 420);
    auto placeMidi = [&](int count, bool inletSide, int y, int x) {
        x += 40;
        for (; count > 0; x += 160, --count)
            addOrganism(inletSide ? pods::kMidiInletClass : pods::kMidiOutletClass,
                           {x, y}, pod);
    };
    placeMidi(midiIns, true, 40, inX);
    placeMidi(midiOuts, false, 420, outX);
    positions_[pod] = at;
    endTransaction();
    return pod;
}

void EngineHost::promotePodStrays(const std::string& pod, int domain,
                                  juce::Point<int> topLeft, juce::Point<int> bottomLeft) {
    const bool midi = domain == 1, video = domain == 2;
    auto& cords = video ? model_.videoConnections
                        : midi ? model_.midiConnections : model_.connections;
    const auto dom = video ? pods::Domain::Video
                           : midi ? pods::Domain::Midi : pods::Domain::Audio;
    for (const bool inletSide : {true, false}) {
        const auto strays = pods::strayRefs(model_, pod, inletSide, dom);
        int k = 0;
        for (const auto& ref : strays) {
            const juce::Point<int> base = inletSide ? topLeft : bottomLeft;
            const char* cls =
                video ? (inletSide ? pods::kVideoInletClass : pods::kVideoOutletClass)
                : midi ? (inletSide ? pods::kMidiInletClass : pods::kMidiOutletClass)
                       : (inletSide ? pods::kInletClass : pods::kOutletClass);
            const auto port = addOrganism(cls, base + juce::Point<int>{160 * k++, 0}, pod);
            if (port.empty()) continue;
            for (auto& c : cords) {
                if (inletSide && c.dst == ref.node && c.dstInlet == ref.chan
                    && !pods::isUnder(c.src, pod)) {
                    c.dst = port;
                    c.dstInlet = 0;
                } else if (!inletSide && c.src == ref.node && c.srcOutlet == ref.chan
                           && !pods::isUnder(c.dst, pod)) {
                    c.src = port;
                    c.srcOutlet = 0;
                }
            }
            ConnectionModel inner;
            if (inletSide) { inner.src = port; inner.dst = ref.node; inner.dstInlet = ref.chan; }
            else           { inner.src = ref.node; inner.srcOutlet = ref.chan; inner.dst = port; }
            cords.push_back(inner);
        }
    }
}

std::string EngineHost::makePod(const std::vector<std::string>& nodes, juce::Point<int> at,
                                const std::string& scope) {
    std::vector<std::string> members;
    for (const auto& n : nodes) {
        if (!pods::inScope(n, scope)) continue;
        if (model_.byName(n) != nullptr || pods::isPod(model_, n)) members.push_back(n);
    }
    if (members.empty()) return {};

    const std::string base = (scope.empty() ? std::string() : scope + "/") + "Pod_";
    std::string pod;
    for (int n = 1;; ++n) {
        pod = base + std::to_string(n);
        if (!model_.byName(pod) && !pods::isPod(model_, pod)) break;
    }

    beginTransaction();
    pushUndo();
    juce::Rectangle<int> bounds;
    bool first = true;
    auto grow = [&](juce::Point<int> p) {
        const juce::Rectangle<int> r(p.x, p.y, 1, 1);
        bounds = first ? r : bounds.getUnion(r);
        first = false;
    };
    for (const auto& n : members) {
        grow(position(n));
        const std::string pre = n + "/";
        for (const auto& c : model_.organisms)
            if (c.name.rfind(pre, 0) == 0) grow(position(c.name));
    }

    std::vector<std::pair<std::string, std::string>> moves;
    for (const auto& n : members) {
        const std::string leaf = pods::leafOf(n);
        moves.emplace_back(n, pod + "/" + leaf);
        const std::string pre = n + "/";
        for (const auto& c : model_.organisms)
            if (c.name.rfind(pre, 0) == 0)
                moves.emplace_back(c.name, pod + "/" + leaf + "/" + c.name.substr(pre.size()));
    }
    for (const auto& m : moves) renameReferences(m.first, m.second);
    for (const auto& n : members)
        if (auto it = positions_.find(n); it != positions_.end()) {
            positions_[pod + "/" + pods::leafOf(n)] = it->second;
            positions_.erase(it);
        }
    positions_[pod] = at;

    const juce::Point<int> above{bounds.getX(), bounds.getY() - 140};
    const juce::Point<int> below{bounds.getX(), bounds.getBottom() + 140};
    promotePodStrays(pod, 0, above, below);
    promotePodStrays(pod, 1, above + juce::Point<int>{0, -80}, below + juce::Point<int>{0, 80});
    promotePodStrays(pod, 2, above + juce::Point<int>{0, -160}, below + juce::Point<int>{0, 160});
    endTransaction();
    return pod;
}

bool EngineHost::renamePod(const std::string& pod, const std::string& newLeaf) {
    if (newLeaf.empty() || newLeaf.find(pods::kSep) != std::string::npos) return false;
    const std::string parent = pods::parentOf(pod);
    const std::string target = parent.empty() ? newLeaf : parent + "/" + newLeaf;
    if (target == pod) return true;
    if (model_.byName(target) || pods::isPod(model_, target)) return false;
    pushUndo();
    const std::string pre = pod + "/";
    std::vector<std::string> inner;
    for (auto& c : model_.organisms)
        if (c.name.rfind(pre, 0) == 0) inner.push_back(c.name);
    for (auto& n : inner)
        renameReferences(n, target + "/" + n.substr(pre.size()));
    if (auto it = positions_.find(pod); it != positions_.end()) {
        positions_[target] = it->second;
        positions_.erase(it);
    }
    requestRebuild();
    return true;
}

void EngineHost::splicePodPorts(const std::string& pod, int domain) {
    const bool midi = domain == 1, video = domain == 2;
    auto& cords = video ? model_.videoConnections
                        : midi ? model_.midiConnections : model_.connections;
    std::vector<std::pair<std::string, int>> ports;
    for (const auto& c : model_.organisms) {
        if (pods::parentOf(c.name) != pod) continue;
        if (!pods::isInletClass(c.displayClass) && !pods::isOutletClass(c.displayClass)) continue;
        const bool isMidi = pods::isMidiPortClass(c.displayClass);
        const bool isVideo = pods::isVideoPortClass(c.displayClass);
        if (midi != isMidi || video != isVideo) continue;
        ports.emplace_back(c.name, midi || video ? 1 : pods::portChannels(c.displayClass));
    }

    for (const auto& [port, channels] : ports) {
        std::vector<ConnectionModel> spliced;
        for (int k = 0; k < channels; ++k) {
            std::vector<const ConnectionModel*> in, out;
            for (const auto& c : cords) {
                if (c.dst == port && c.dstInlet == k) in.push_back(&c);
                if (c.src == port && c.srcOutlet == k) out.push_back(&c);
            }
            for (const auto* a : in)
                for (const auto* b : out) {
                    ConnectionModel n;
                    n.src = a->src;
                    n.srcOutlet = a->srcOutlet;
                    n.dst = b->dst;
                    n.dstInlet = b->dstInlet;
                    if (midi) {
                        if (a->midiChannel != 0 && b->midiChannel != 0
                            && a->midiChannel != b->midiChannel) continue;
                        n.midiChannel = a->midiChannel != 0 ? a->midiChannel : b->midiChannel;
                    }
                    spliced.push_back(n);
                }
        }
        cords.erase(std::remove_if(cords.begin(), cords.end(),
                                   [&p = port](const ConnectionModel& c) {
                                       return c.src == p || c.dst == p;
                                   }),
                    cords.end());
        for (auto& n : spliced) cords.push_back(std::move(n));
        auto& cs = model_.organisms;
        cs.erase(std::remove_if(cs.begin(), cs.end(),
                                [&p = port](const OrganismModel& c) { return c.name == p; }),
                 cs.end());
        positions_.erase(port);
    }
}

void EngineHost::ungroupPod(const std::string& pod) {
    if (!pods::isPod(model_, pod)) return;
    beginTransaction();
    pushUndo();
    for (int dom = 0; dom < 3; ++dom) splicePodPorts(pod, dom);

    const std::string scope = pods::parentOf(pod);
    const std::string prefix = scope.empty() ? std::string() : scope + "/";
    const std::string pre = pod + "/";
    std::vector<std::string> directChildren;
    for (const auto& c : model_.organisms) {
        if (c.name.rfind(pre, 0) != 0) continue;
        const auto rest = c.name.substr(pre.size());
        const auto i = rest.find(pods::kSep);
        const auto seg = i == std::string::npos ? rest : rest.substr(0, i);
        if (std::find(directChildren.begin(), directChildren.end(), seg) == directChildren.end())
            directChildren.push_back(seg);
    }
    std::vector<std::pair<std::string, std::string>> moves;
    for (const auto& seg : directChildren) {
        std::string target = prefix + seg;
        for (int n = 2; model_.byName(target) != nullptr || pods::isPod(model_, target); ++n)
            target = prefix + seg + "_" + std::to_string(n);
        const std::string from = pre + seg;
        for (const auto& c : model_.organisms) {
            if (c.name == from) moves.emplace_back(c.name, target);
            else if (c.name.rfind(from + "/", 0) == 0)
                moves.emplace_back(c.name, target + c.name.substr(from.size()));
        }
    }
    for (const auto& m : moves) renameReferences(m.first, m.second);
    positions_.erase(pod);
    endTransaction();
    requestRebuild();
}

void EngineHost::deletePod(const std::string& pod) {
    pushUndo();
    const std::string pre = pod + "/";
    auto under = [&](const std::string& n) { return n.rfind(pre, 0) == 0; };
    auto& cs = model_.organisms;
    cs.erase(std::remove_if(cs.begin(), cs.end(),
                            [&](const OrganismModel& c) { return under(c.name); }), cs.end());
    for (auto* list : {&model_.connections, &model_.midiConnections, &model_.videoConnections})
        list->erase(std::remove_if(list->begin(), list->end(),
                                   [&](const ConnectionModel& c) {
                                       return under(c.src) || under(c.dst);
                                   }),
                    list->end());
    for (auto it = positions_.begin(); it != positions_.end();)
        it = (under(it->first) || it->first == pod) ? positions_.erase(it) : std::next(it);
    requestRebuild();
}

}
