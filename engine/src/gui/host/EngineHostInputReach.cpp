// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/graph/AudioGraph.h"
#include "core/packs/Roles.h"
#include "core/plugins/PluginNode.h"
#include "hum/caps/Files.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Video.h"

namespace hum {

static_assert(OrganismModel::kTrackInputAll == kLiveMidiAllPorts);

bool EngineHost::nodeIsMidiTrack(const std::string& name) const {
    const auto* cm = model_.byName(name);
    return cm != nullptr && classHasRole(cm->classRaw, role::kMidiTrack);
}

bool EngineHost::nodeHoldsNotes(const std::string& name) {
    if (graph_ == nullptr) return false;
    auto* live = graph_->find(name);
    return dynamic_cast<ClipArrangement*>(live) != nullptr && dynamic_cast<ClipRecorder*>(live) == nullptr;
}

bool EngineHost::nodePassesMidiOn(const std::string& name) {
    if (graph_ == nullptr) return false;
    auto* live = graph_->find(name);
    auto* midi = dynamic_cast<MidiNode*>(live);
    return midi != nullptr && midi->numMidiOutputs() > 0 && live->numAudioOutputs() == 0;
}

bool EngineHost::nodePlaysNotes(const std::string& name) {
    if (graph_ == nullptr) return false;
    auto* live = graph_->find(name);
    auto* midi = dynamic_cast<MidiNode*>(live);
    if (midi == nullptr || midi->numMidiInputs() <= 0) return false;
    if (dynamic_cast<ClipRecorder*>(live) != nullptr) return false;
    auto* video = dynamic_cast<VideoNode*>(live);
    return live->numAudioOutputs() > 0 || (video != nullptr && video->numVideoOutputs() > 0);
}

std::string EngineHost::trackFeeding(const std::string& instrument) {
    for (const auto& cm : model_.organisms) {
        if (!nodeIsMidiTrack(cm.name)) continue;
        for (const auto& cord : model_.midiConnections)
            if (cord.src == cm.name && cord.dst == instrument) return cm.name;
    }
    return {};
}

std::string EngineHost::makeTrackFor(const std::string& instrument, const std::vector<int>& ports) {
    if (model_.byName(instrument) == nullptr) return {};
    auto track = addOrganism(classWithRole(role::kMidiTrack), position(instrument).translated(-160, 0));
    if (track.empty()) return {};
    if (renameOrganism(track, instrument + " MIDI")) track = instrument + " MIDI";
    connectMidi(track, 0, instrument, 0);
    patterns().ensureNote(track);
    if (ports.size() == 1 && (ports.front() >= 0 || ports.front() == OrganismModel::kTrackInputAll))
        midi().setTrackInput(track, ports.front());
    return track;
}

std::vector<std::string> EngineHost::ensureTracksFeeding(const std::vector<InputReach>& played) {
    std::vector<std::string> out;
    std::vector<const InputReach*> missing;
    for (const auto& reach : played) {
        const auto had = trackFeeding(reach.instrument);
        if (had.empty()) missing.push_back(&reach);
        else if (std::find(out.begin(), out.end(), had) == out.end()) out.push_back(had);
    }
    if (missing.empty()) return out;
    MidiOnlyEditScope quiet(*this);
    beginTransaction();
    for (const auto* reach : missing)
        if (const auto made = makeTrackFor(reach->instrument, reach->ports); !made.empty()) out.push_back(made);
    endTransaction();
    return out;
}

std::vector<InputReach> EngineHost::rowsReachedByInput() {
    std::map<std::string, std::set<int>> tracks, instruments;
    if (graph_ == nullptr) return {};
    auto land = [&](const std::string& node, int port) {
        if (nodeIsMidiTrack(node) || nodeHoldsNotes(node)) {
            tracks[node].insert(port);
            return true;
        }
        if (nodePlaysNotes(node)) {
            instruments[node].insert(port);
            return true;
        }
        return false;
    };
    for (const auto& cm : model_.organisms) {
        if (!classHasRole(cm.classRaw, role::kMidiIn)) continue;
        auto* in = dynamic_cast<LiveMidiIn*>(graph_->find(cm.name));
        if (in == nullptr) continue;
        const int port = in->liveMidiPort();
        std::set<std::string> seen{cm.name};
        std::vector<std::string> frontier{cm.name};
        while (!frontier.empty()) {
            const auto at = frontier.back();
            frontier.pop_back();
            for (const auto& cord : model_.midiConnections) {
                if (cord.src != at || !seen.insert(cord.dst).second) continue;
                if (!land(cord.dst, port) && nodePassesMidiOn(cord.dst)) frontier.push_back(cord.dst);
            }
        }
    }
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        for (const auto& t : midiState_.targets) {
            if (t.mode == OrganismModel::kMidiCordsOnly) continue;
            for (const auto& cm : model_.organisms)
                if (dynamic_cast<PluginNode*>(graph_->find(cm.name)) == t.hp) land(cm.name, kLiveMidiAllPorts);
        }
    }
    std::vector<InputReach> out;
    for (const auto& [track, ports] : tracks) out.push_back({track, std::vector<int>(ports.begin(), ports.end()), {}});
    for (const auto& [instrument, ports] : instruments)
        out.push_back({{}, std::vector<int>(ports.begin(), ports.end()), instrument});
    return out;
}

void EngineHost::refreshArmedInputs() {
    std::set<std::string> corded;
    std::map<std::string, std::vector<int>> played;
    for (const auto& r : rowsReachedByInput()) {
        if (!r.node.empty()) corded.insert(r.node);
        else played[r.instrument] = r.ports;
    }
    std::vector<std::string> tapped;
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        auto& v = midiState_.recordTargets;
        v.erase(std::remove_if(v.begin(), v.end(), [&](const MidiState::RecordTarget& t) {
            return !t.node.empty() && model_.byName(t.node) == nullptr;
        }), v.end());
        for (auto& t : v) {
            if (!t.inlet) continue;
            tapped.push_back(t.node);
            t.thru = false;
            t.fromInput = false;
            t.ports.clear();
            const auto* cm = model_.byName(t.node);
            const int input = cm != nullptr ? cm->trackInput : OrganismModel::kTrackInputAuto;
            if (corded.count(t.node) != 0 || input == OrganismModel::kTrackInputNone) continue;
            std::vector<int> playedPorts;
            for (const auto& cord : model_.midiConnections)
                if (const auto it = played.find(cord.dst); cord.src == t.node && it != played.end())
                    playedPorts.insert(playedPorts.end(), it->second.begin(), it->second.end());
            t.fromInput = true;
            if (input == OrganismModel::kTrackInputAuto) {
                t.ports = playedPorts.empty() ? std::vector<int>{kLiveMidiAllPorts} : playedPorts;
                t.thru = playedPorts.empty();
            } else {
                t.ports = {input};
                t.thru = std::none_of(playedPorts.begin(), playedPorts.end(),
                                      [&](int p) { return p == kLiveMidiAllPorts || p == input; });
            }
        }
    }
    if (graph_ == nullptr) return;
    const juce::ScopedLock sl(lock_);
    for (const auto& node : tapped) graph_->setMidiInletTap(graph_->indexOf(node), true);
}

}
