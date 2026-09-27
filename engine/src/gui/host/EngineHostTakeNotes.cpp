// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

#include "core/graph/AudioGraph.h"
#include "core/midi/MidiRecord.h"
#include "core/packs/Roles.h"
#include "gui/tracks/ClipColors.h"
#include "gui/tracks/LooperFlow.h"

namespace hum {

namespace {

bool targetAccepts(const OrganismModel* cm, unsigned char status) {
    if (!cm) return true;
    return captureAccepts(cm->midiReceiveMode, cm->midiReceiveChannel, status);
}

bool isNoteOn(const TimedMidi& e) { return (e.status & 0xF0) == 0x90 && e.d2 > 0; }

int takeColourFor(const std::vector<NoteEvent>& existing) {
    if (existing.empty()) return kNoColour;
    std::vector<int> used;
    for (const auto& n : existing) used.push_back(n.colour);
    for (int i = 1; i <= kNumClipColors; ++i) {
        const int rgb = (int) (clipColour(i).getARGB() & 0xFFFFFFu);
        if (std::find(used.begin(), used.end(), rgb) == used.end()) return rgb;
    }
    return (int) (clipColour(1 + (int) (used.size() % (size_t) kNumClipColors)).getARGB() & 0xFFFFFFu);
}

}

void EngineHost::drainInletTap(std::vector<TimedMidi>& into) {
    if (graph_ == nullptr) return;
    std::vector<TappedMidi> tapped;
    graph_->midiInletTap().drainInto(tapped);
    for (const auto& t : tapped) {
        TimedMidi e;
        e.beat = t.beat;
        e.status = t.status;
        e.d1 = t.d1;
        e.d2 = t.d2;
        e.node = t.node;
        into.push_back(e);
    }
}

std::vector<TimedMidi> EngineHost::carryUnfinishedNotes(const std::vector<TimedMidi>& events, bool finalize) {
    std::map<std::pair<int, int>, std::vector<TimedMidi>> streams;
    for (const auto& e : events) streams[{e.node, e.port}].push_back(e);
    std::vector<TimedMidi> carried;
    for (const auto& [key, stream] : streams) {
        std::vector<TimedMidi> remaining;
        assembleRecordedNotes(stream, 4 * 4 * Pattern::kTicksPerBeat, 0, finalize, Pattern::kTicksPerBeat / 4,
                              remaining);
        carried.insert(carried.end(), remaining.begin(), remaining.end());
    }
    if (!carried.empty()) {
        const juce::ScopedLock ml(midiState_.targetsLock);
        midiState_.capture.insert(midiState_.capture.begin(), carried.begin(), carried.end());
    }
    return carried;
}

void EngineHost::flushMidiRecording(bool finalize) {
    std::vector<MidiState::RecordTarget> targets;
    std::vector<TimedMidi> events;
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        if (midiState_.recordTargets.empty()) return;
        events.swap(midiState_.capture);
    }
    drainInletTap(events);
    if (events.empty()) return;
    std::stable_sort(events.begin(), events.end(),
                     [](const TimedMidi& a, const TimedMidi& b) { return a.beat < b.beat; });
    {
        const juce::ScopedLock ml(midiState_.targetsLock);
        targets = midiState_.recordTargets;
    }
    const bool takeRunning = record_.sessionActive();
    auto takes = [takeRunning](const MidiState::RecordTarget& t) { return !t.inlet || takeRunning; };
    if (std::none_of(targets.begin(), targets.end(), takes)) return;
    carryUnfinishedNotes(events, finalize);

    const int barTicks = automation().timeSigNumerator() * Pattern::kTicksPerBeat;
    const double nowBeat = liveBeats_.load();
    std::vector<std::string> dead;
    for (auto& tg : targets) {
        if (!takes(tg)) continue;
        const int graphIndex = tg.node.empty() || graph_ == nullptr ? -1 : graph_->indexOf(tg.node);
        auto* cm = tg.node.empty() ? nullptr : model_.byName(tg.node);
        std::vector<TimedMidi> mine;
        for (const auto& e : events)
            if (tg.hears(e, graphIndex) && targetAccepts(cm, e.status)) mine.push_back(e);
        if (mine.empty()) continue;

        if (cm == nullptr) { dead.push_back(tg.node); continue; }

        if (tg.clip == kClipOnDemand) {
            double first = -1.0;
            for (const auto& e : mine)
                if (isNoteOn(e)) { first = e.beat; break; }
            if (first < 0.0) continue;
            if (!midiState_.recordUndoPushed) { pushUndo(); midiState_.recordUndoPushed = true; }
            const int firstTick = (int) std::lround(first * Pattern::kTicksPerBeat);
            const auto existing = clips().list(tg.node);
            int reuse = -1;
            for (const auto& ci : existing)
                if (!ci.hasMedia() && firstTick >= ci.startTick && firstTick < ci.startTick + ci.lengthTicks)
                    reuse = ci.index;
            if (reuse >= 0) {
                tg.clip = reuse;
                tg.takeColour = takeColourFor(clips().notes(tg.node, reuse));
            } else {
                const int start = (int) std::floor(first * Pattern::kTicksPerBeat / barTicks) * barTicks;
                tg.clip = clips().add(tg.node, start, barTicks);
            }
            tg.grow = true;
            const juce::ScopedLock ml(midiState_.targetsLock);
            for (auto& live : midiState_.recordTargets)
                if (live.node == tg.node && live.clip == kClipOnDemand) {
                    live.clip = tg.clip;
                    live.grow = true;
                    live.takeColour = tg.takeColour;
                }
        }

        double offsetBeats = 0.0;
        bool wrap = true;
        int dur = cm->pattern.present && cm->pattern.duration > 0 ? cm->pattern.duration
                                                                  : 4 * 4 * Pattern::kTicksPerBeat;
        if (tg.clip >= 0) {
            const auto cs = clips().list(tg.node);
            if (tg.clip >= (int) cs.size()) { dead.push_back(tg.node); continue; }
            const auto& ci = cs[(size_t) tg.clip];
            if (tg.grow) {
                double last = nowBeat;
                for (const auto& e : mine) last = std::max(last, e.beat);
                const int need = (int) std::ceil((last * Pattern::kTicksPerBeat - ci.startTick) / barTicks) * barTicks;
                if (need > ci.lengthTicks) clips().resize(tg.node, tg.clip, need, false);
            }
            const auto grown = clips().list(tg.node)[(size_t) tg.clip];
            offsetBeats = grown.startTick / (double) Pattern::kTicksPerBeat;
            wrap = grown.looped;
            dur = grown.lengthTicks;
        }
        std::vector<TimedMidi> unused;
        auto fresh = assembleRecordedNotes(mine, dur, tg.quantize, finalize,
                                           tg.quantize > 0 ? tg.quantize : Pattern::kTicksPerBeat / 4, unused,
                                           offsetBeats, wrap);
        if (fresh.empty()) continue;
        for (auto& n : fresh) n.colour = tg.takeColour;
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


}
