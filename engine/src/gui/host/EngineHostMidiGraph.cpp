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
            if (port < 0 || in->liveMidiPort() == port) in->pushLiveMidi(ev);
    if (!midiState_.recordTargets.empty() && playing_ && routable
        && (m.isNoteOn() || m.isNoteOff())) {
        TimedMidi t;
        t.beat = liveBeats_.load();
        t.status = ev.data[0]; t.d1 = ev.data[1]; t.d2 = ev.data[2];
        midiState_.capture.push_back(t);
        for (const auto& tg : midiState_.recordTargets)
            if (tg.thru)
                for (auto& [nm, in] : namedLiveIns_)
                    if (nm == tg.node) { in->pushLiveMidi(ev); break; }
    }
    if (routable)
        for (const auto& target : liveTargets_) {
            bool already = false;
            if (playing_ && (m.isNoteOn() || m.isNoteOff()))
                for (const auto& tg : midiState_.recordTargets)
                    if (tg.thru && tg.node == target) already = true;
            if (already) continue;
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

namespace {
bool targetAccepts(const OrganismModel* cm, unsigned char status) {
    if (!cm) return true;
    return captureAccepts(cm->midiReceiveMode, cm->midiReceiveChannel, status);
}
}

void EngineHost::flushMidiRecording(bool finalize) {
    std::vector<MidiState::RecordTarget> targets;
    std::vector<TimedMidi> events;
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        if (midiState_.recordTargets.empty() || midiState_.capture.empty()) return;
        targets = midiState_.recordTargets;
        events.swap(midiState_.capture);
    }
    {
        std::vector<TimedMidi> remaining;
        assembleRecordedNotes(events, 4 * 4 * Pattern::kTicksPerBeat, 0, finalize,
                              Pattern::kTicksPerBeat / 4, remaining);
        if (!remaining.empty()) {
            const juce::ScopedLock ml(midiState_.targetsLock);
            midiState_.capture.insert(midiState_.capture.begin(), remaining.begin(), remaining.end());
        }
    }

    const int barTicks = automation().timeSigNumerator() * Pattern::kTicksPerBeat;
    const double nowBeat = liveBeats_.load();
    for (auto& tg : targets) {
        if (tg.clip == kClipOnDemand) {
            double first = -1.0;
            for (const auto& e : events)
                if ((e.status & 0xF0) == 0x90 && e.d2 > 0) { first = e.beat; break; }
            if (first < 0.0) continue;
            if (!midiState_.recordUndoPushed) { pushUndo(); midiState_.recordUndoPushed = true; }
            const int start = (int) std::floor(first * Pattern::kTicksPerBeat / barTicks) * barTicks;
            tg.clip = clips().add(tg.node, start, barTicks);
            tg.grow = true;
            const juce::ScopedLock ml(midiState_.targetsLock);
            for (auto& live : midiState_.recordTargets)
                if (live.node == tg.node && live.clip == kClipOnDemand) {
                    live.clip = tg.clip;
                    live.grow = true;
                }
        }
        if (!tg.grow || tg.clip < 0) continue;
        const auto cs = clips().list(tg.node);
        if (tg.clip >= (int) cs.size()) continue;
        const auto& ci = cs[(size_t) tg.clip];
        double last = nowBeat;
        for (const auto& e : events) last = std::max(last, e.beat);
        const int need = (int) std::ceil((last * Pattern::kTicksPerBeat - ci.startTick) / barTicks)
                         * barTicks;
        if (need > ci.lengthTicks) clips().resize(tg.node, tg.clip, need, false);
    }

    std::vector<std::string> dead;
    for (const auto& tg : targets) {
        if (tg.clip == kClipOnDemand) continue;
        auto* cm = model_.byName(tg.node);
        if (!cm) { dead.push_back(tg.node); continue; }
        double offsetBeats = 0.0;
        bool wrap = true;
        int dur = cm->pattern.present && cm->pattern.duration > 0
                      ? cm->pattern.duration : 4 * 4 * Pattern::kTicksPerBeat;
        if (tg.clip >= 0) {
            const auto cs = clips().list(tg.node);
            if (tg.clip >= (int) cs.size()) { dead.push_back(tg.node); continue; }
            offsetBeats = cs[(size_t) tg.clip].startTick / (double) Pattern::kTicksPerBeat;
            wrap = cs[(size_t) tg.clip].looped;
            dur = cs[(size_t) tg.clip].lengthTicks;
        }
        std::vector<TimedMidi> filtered;
        for (const auto& e : events)
            if (targetAccepts(cm, e.status)) filtered.push_back(e);
        if (filtered.empty()) continue;
        std::vector<TimedMidi> unused;
        auto fresh = assembleRecordedNotes(filtered, dur, tg.quantize, finalize,
                                           tg.quantize > 0 ? tg.quantize : Pattern::kTicksPerBeat / 4,
                                           unused, offsetBeats, wrap);
        if (fresh.empty()) continue;
        if (!midiState_.recordUndoPushed) { pushUndo(); midiState_.recordUndoPushed = true; }
        if (tg.clip >= 0) {
            auto all = clips().notes(tg.node, tg.clip);
            all.insert(all.end(), fresh.begin(), fresh.end());
            clips().setNotes(tg.node, tg.clip, all, 0);
        } else {
            auto all = patterns().noteEvents(tg.node);
            all.insert(all.end(), fresh.begin(), fresh.end());
            patterns().setNoteEvents(tg.node, all, dur);
        }
    }
    if (!dead.empty()) {
        const juce::ScopedLock ml(midiState_.targetsLock);
        auto& v = midiState_.recordTargets;
        v.erase(std::remove_if(v.begin(), v.end(), [&](const MidiState::RecordTarget& t) {
            return std::find(dead.begin(), dead.end(), t.node) != dead.end();
        }), v.end());
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
