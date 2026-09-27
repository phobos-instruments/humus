// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <cmath>

#include "core/graph/GraphMidi.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "gui/tracks/LooperFlow.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
bool toMidiEvent(const juce::MidiMessage& m, MidiEvent& out) {
    return graphmidi::toEvent(m, out);
}
}

void EngineHost::routeLiveMidi(const juce::MidiMessage& m, int port) {
    MidiEvent ev;
    const bool routable = toMidiEvent(m, ev);
    const juce::ScopedLock ml(midiState_.targetsLock);
    for (const auto& t : midiState_.targets) {
        if (t.mode == OrganismModel::kMidiCordsOnly) continue;
        if (t.mode == OrganismModel::kMidiChannel
            && m.getChannel() != 0 && m.getChannel() != t.channel) continue;
        t.hp->queueMidiMessage(m);
    }
    if (routable)
        for (auto* in : liveMidiIns_)
            if (port < 0 || in->liveMidiPort() == kLiveMidiAllPorts || in->liveMidiPort() == port)
                in->pushLiveMidi(ev);
    const bool note = m.isNoteOn() || m.isNoteOff();
    auto sounds = [&](const std::string& node) {
        return !m.isNoteOn() || midiState_.silenced.count(node) == 0;
    };
    auto thruNow = [&](const MidiState::RecordTarget& tg) {
        return tg.thru && tg.hearsPort(port) && (tg.inlet || (playing_ && note));
    };
    if (!routable) return;
    if (m.isNoteOn())
        for (const auto& tg : midiState_.recordTargets)
            if (tg.inlet && tg.hearsPort(port)) ++midiState_.keyboardHits[tg.node];
    for (const auto& tg : midiState_.recordTargets)
        if (thruNow(tg) && sounds(tg.node))
            for (auto& [nm, in] : namedLiveIns_)
                if (nm == tg.node) { in->pushLiveMidi(ev); break; }
    if (!midiState_.recordTargets.empty() && playing_ && note) {
        TimedMidi t;
        t.beat = liveBeats_.load();
        t.status = ev.data[0]; t.d1 = ev.data[1]; t.d2 = ev.data[2];
        t.port = port;
        midiState_.capture.push_back(t);
    }
    for (const auto& target : liveTargets_) {
        const bool already = std::any_of(midiState_.recordTargets.begin(), midiState_.recordTargets.end(),
                                          [&](const MidiState::RecordTarget& tg) { return thruNow(tg) && tg.node == target; });
        if (already || !sounds(target)) continue;
        for (auto& [nm, in] : namedLiveIns_)
            if (nm == target) { in->pushLiveMidi(ev); break; }
    }
}

void EngineHost::pushToMonitors(const juce::MidiMessage& m, int port) {
    MidiEvent ev;
    if (!toMidiEvent(m, ev)) return;
    const juce::ScopedLock ml(midiState_.targetsLock);
    for (auto* mon : midiMonitors_) mon->pushLiveMidi(ev, port);
}

void EngineHost::injectLiveMidi(const juce::MidiMessage& m) {
    routeLiveMidi(m);
}

void EngineHost::finishOnDemandClips() {
    flushMidiRecording(true);
    const juce::ScopedLock ml(midiState_.targetsLock);
    for (auto& t : midiState_.recordTargets)
        if (t.grow) { t.clip = kClipOnDemand; t.grow = false; }
}

void EngineHost::injectLiveMidiToNode(const std::string& node, const juce::MidiMessage& m) {
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        if (graph_)
            if (auto* c = graph_->find(node)) {
                if (auto* in = dynamic_cast<LiveMidiIn*>(c)) {
                    MidiEvent ev;
                    if (toMidiEvent(m, ev)) in->pushLiveMidi(ev);
                    if (!midiState_.recordTargets.empty() && playing_
                        && (m.isNoteOn() || m.isNoteOff())) {
                        TimedMidi t;
                        t.beat = liveBeats_.load();
                        t.status = ev.data[0]; t.d1 = ev.data[1]; t.d2 = ev.data[2];
                        midiState_.capture.push_back(t);
                    }
                    return;
                }
                if (auto* pn = dynamic_cast<PluginNode*>(c)) {
                    pn->queueMidiMessage(m);
                    return;
                }
            }
    }
    if (graph_) {
        const int idx = graph_->indexOf(node);
        if (graph_->acceptsLiveMidi(idx)) {
            MidiEvent ev;
            if (toMidiEvent(m, ev)) {
                {
                    const juce::ScopedLock ml(midiState_.targetsLock);
                    if (!midiState_.recordTargets.empty() && playing_
                        && (m.isNoteOn() || m.isNoteOff())) {
                        TimedMidi t;
                        t.beat = liveBeats_.load();
                        t.status = ev.data[0]; t.d1 = ev.data[1]; t.d2 = ev.data[2];
                        midiState_.capture.push_back(t);
                    }
                }
                graph_->pushLiveMidi(idx, ev);
                return;
            }
        }
    }
    routeLiveMidi(m);
}


void EngineHost::panic() {
    {
        const juce::ScopedLock sl(lock_);
        if (graph_ != nullptr) graph_->panic();
    }
    const juce::ScopedLock ml(midiState_.targetsLock);
    for (int port = 0; port < MidiState::kPorts; ++port) {
        auto* out = midiState_.outForPort(port);
        if (out == nullptr) continue;
        for (int channel = 1; channel <= kMidiChannels; ++channel) {
            for (const int cc : {kSustainController, kAllSoundOffController, kAllNotesOffController})
                out->sendMessageNow(juce::MidiMessage::controllerEvent(channel, cc, 0));
            out->sendMessageNow(juce::MidiMessage::pitchWheel(channel, kBendCentre));
        }
    }
}

void EngineHost::pollMidiOut() {
    MidiEvent buf[128];
    juce::MidiBuffer blocks[kMidiPorts];
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        for (auto* drain : midiDrains_) {
            const int port = drain->pendingMidiPort();
            const int n = drain->consumeOutput(buf, 128);
            if (midiState_.outForPort(port) == nullptr) continue;
            for (int i = 0; i < n; ++i) {
                if (buf[i].size < 1) continue;
                blocks[port].addEvent(graphmidi::toMessage(buf[i]),
                                      juce::jmax(0, buf[i].sampleOffset));
            }
        }
    }
    for (int p = 0; p < kMidiPorts; ++p) {
        if (blocks[p].isEmpty()) continue;
        midiState_.outs[p]->sendBlockOfMessages(blocks[p],
                                          juce::Time::getMillisecondCounterHiRes() + 1.0,
                                          sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate);
    }
}

}
