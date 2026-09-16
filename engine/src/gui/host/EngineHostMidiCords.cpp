// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <algorithm>
#include "gui/host/EngineHost.h"
#include "io/ModRouteBuild.h"
#include "core/params/ParamSchema.h"
#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"
#include "core/packs/Roles.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Video.h"
#include "hum/Registry.h"

namespace hum {

void EngineHost::connectMidi(const std::string& src, int srcPort,
                             const std::string& dst, int dstPort) {
    if (src == dst) return;
    if (isMidiConnected(src, srcPort, dst, dstPort)) return;
    if (cordWouldStray(src, srcPort, dst, dstPort, pods::Domain::Midi)) return;
    pushUndo();
    model_.midiConnections.push_back({src, srcPort, dst, dstPort});
    requestRebuild();
}

void EngineHost::removeMidiConnection(const std::string& src, int srcPort,
                                      const std::string& dst, int dstPort) {
    pushUndo();
    auto& cn = model_.midiConnections;
    cn.erase(std::remove_if(cn.begin(), cn.end(), [&](const ConnectionModel& c) {
        return c.src == src && c.srcOutlet == srcPort && c.dst == dst && c.dstInlet == dstPort;
    }), cn.end());
    requestRebuild();
}

void EngineHost::disconnectMidiInto(const std::string& dst, int dstPort) {
    pushUndo();
    auto& cn = model_.midiConnections;
    cn.erase(std::remove_if(cn.begin(), cn.end(),
                            [&](const ConnectionModel& c) { return c.dst == dst && c.dstInlet == dstPort; }), cn.end());
    requestRebuild();
}

void EngineHost::applyMidiTrackTarget(const std::string& name, int value) {
    std::string want;
    if (value > 1)
        for (const auto& it : choiceItems("midi-targets", name))
            if (it.first == value) { want = it.second; break; }
    std::vector<ConnectionModel> old;
    for (const auto& c : model_.midiConnections)
        if (c.src == name && c.srcOutlet == 0 && c.dst != want) old.push_back(c);
    for (const auto& c : old)
        removeMidiConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    if (!want.empty()) connectMidi(name, 0, want, 0);
}

void EngineHost::syncMidiTrackTargets() {
    for (const auto& cm : model_.organisms) {
        if (!classHasRole(cm.classRaw, role::kMidiTrack)) continue;
        std::string dst;
        for (const auto& c : model_.midiConnections)
            if (c.src == cm.name && c.srcOutlet == 0) { dst = c.dst; break; }
        double want = 1.0;
        if (!dst.empty())
            for (const auto& it : choiceItems("midi-targets", cm.name))
                if (it.second == dst) { want = (double) it.first; break; }
        double current = 1.0;
        for (const auto& p : cm.properties)
            if (p.name == "Target") { current = p.value; break; }
        if (current != want) setParam(cm.name, "Target", want);
    }
}

bool EngineHost::isMidiConnected(const std::string& src, int srcPort,
                                 const std::string& dst, int dstPort) const {
    for (auto& c : model_.midiConnections)
        if (c.src == src && c.srcOutlet == srcPort && c.dst == dst && c.dstInlet == dstPort)
            return true;
    return false;
}

void EngineHost::setMidiCordChannel(const std::string& src, int srcPort,
                                    const std::string& dst, int dstPort, int channel) {
    for (auto& c : model_.midiConnections)
        if (c.src == src && c.srcOutlet == srcPort && c.dst == dst && c.dstInlet == dstPort) {
            const int ch = juce::jlimit(0, 16, channel);
            if (c.midiChannel == ch) return;
            pushUndo();
            c.midiChannel = ch;
            requestRebuild();
            return;
        }
}

int EngineHost::midiCordChannel(const std::string& src, int srcPort,
                                const std::string& dst, int dstPort) const {
    for (auto& c : model_.midiConnections)
        if (c.src == src && c.srcOutlet == srcPort && c.dst == dst && c.dstInlet == dstPort)
            return c.midiChannel;
    return 0;
}

int EngineHost::midiInletsOf(const std::string& name) {
    int wired = 0;
    for (auto& c : model_.midiConnections)
        if (c.dst == name) wired = std::max(wired, c.dstInlet + 1);
    if (graph_)
        if (auto* m = dynamic_cast<MidiNode*>(graph_->find(name)))
            return std::max(m->numMidiInputs(), wired);
    return wired;
}

int EngineHost::midiOutletsOf(const std::string& name) {
    int wired = 0;
    for (auto& c : model_.midiConnections)
        if (c.src == name) wired = std::max(wired, c.srcOutlet + 1);
    if (graph_)
        if (auto* m = dynamic_cast<MidiNode*>(graph_->find(name)))
            return std::max(m->numMidiOutputs(), wired);
    return wired;
}

void EngineHost::connectVideo(const std::string& src, int srcPort,
                              const std::string& dst, int dstPort) {
    if (src == dst) return;
    ConnectionModel c{src, srcPort, dst, dstPort};
    pods::resolvePodVideoCord(model_, c);
    if (cordWouldStray(c.src, c.srcOutlet, c.dst, c.dstInlet, pods::Domain::Video)) return;
    if (isVideoConnected(c.src, c.srcOutlet, c.dst, c.dstInlet)) return;
    pushUndo();
    model_.videoConnections.push_back(c);
    requestRebuild();
}

void EngineHost::removeVideoConnection(const std::string& src, int srcPort,
                                       const std::string& dst, int dstPort) {
    pushUndo();
    auto& cn = model_.videoConnections;
    cn.erase(std::remove_if(cn.begin(), cn.end(), [&](const ConnectionModel& c) {
        return c.src == src && c.srcOutlet == srcPort && c.dst == dst && c.dstInlet == dstPort;
    }), cn.end());
    requestRebuild();
}

bool EngineHost::isVideoConnected(const std::string& src, int srcPort,
                                  const std::string& dst, int dstPort) const {
    for (auto& c : model_.videoConnections)
        if (c.src == src && c.srcOutlet == srcPort && c.dst == dst && c.dstInlet == dstPort)
            return true;
    return false;
}

std::string EngineHost::videoSourceInto(const std::string& dst, int dstPort) const {
    int outlet = 0;
    return videoSourceInto(dst, dstPort, outlet);
}

std::string EngineHost::videoSourceInto(const std::string& dst, int dstPort,
                                        int& srcOutlet) const {
    for (auto& c : model_.videoConnections)
        if (c.dst == dst && c.dstInlet == dstPort) {
            srcOutlet = c.srcOutlet;
            return c.src;
        }
    srcOutlet = 0;
    return {};
}

int EngineHost::videoInletsOf(const std::string& name) {
    int wired = 0;
    for (auto& c : model_.videoConnections)
        if (c.dst == name) wired = std::max(wired, c.dstInlet + 1);
    if (graph_)
        if (auto* v = dynamic_cast<VideoNode*>(graph_->find(name)))
            return std::max(v->numVideoInputs(), wired);
    return wired;
}

int EngineHost::videoOutletsOf(const std::string& name) {
    int wired = 0;
    for (auto& c : model_.videoConnections)
        if (c.src == name) wired = std::max(wired, c.srcOutlet + 1);
    if (graph_)
        if (auto* v = dynamic_cast<VideoNode*>(graph_->find(name)))
            return std::max(v->numVideoOutputs(), wired);
    return wired;
}

}
