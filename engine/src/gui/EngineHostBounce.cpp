#include "gui/EngineHost.h"

#include <algorithm>
#include <cmath>

#include "core/CordTrace.h"
#include "core/ParamSchema.h"
#include "core/RecordTake.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"
#include "core/MidiRecord.h"
#include "hum/NoteSchedule.h"

namespace hum {

std::string EngineHost::bounceSourceOf(const std::string& node) {
    if (outletsOf(node) > 0) return node;
    for (auto n = cords::destinationOf(model_, node); !n.empty();
         n = cords::destinationOf(model_, n))
        if (outletsOf(n) > 0) return n;
    for (const auto& t : noteTargets(node)) {
        if (outletsOf(t) > 0) return t;
        for (auto n = cords::destinationOf(model_, t); !n.empty();
             n = cords::destinationOf(model_, n))
            if (outletsOf(n) > 0) return n;
    }
    return {};
}

void EngineHost::primeNoteTracks(AudioGraph& g) const {
    for (const auto& cm : model_.organisms) {
        const int ni = g.indexOf(cm.name);
        if (ni < 0) continue;
        bool noteTrack = false;
        for (const auto& ch : cm.pattern.channels)
            if (ch.type == "note-events") noteTrack = true;
        if (!noteTrack) continue;
        auto* mn = dynamic_cast<MidiNode*>(g.find(cm.name));
        if (mn == nullptr || mn->numMidiInputs() <= 0) continue;
        if (dynamic_cast<ClipArrangement*>(g.find(cm.name)) != nullptr) continue;
        g.setNodeTrack(ni, noteschedule::prepare(cm.pattern, 4 * 4 * Pattern::kTicksPerBeat));
        g.setNodeTrackMuted(ni, modelTrackMuted(cm));
    }
}

std::string EngineHost::consolidate(const std::string& node, double fromBeat,
                                    double toBeat, std::string& error) {
    if (toBeat <= fromBeat) { error = "nothing selected to consolidate"; return {}; }
    const auto source = bounceSourceOf(node);
    if (source.empty()) { error = node + " does not reach anything that makes sound"; return {}; }

    AudioGraph g;
    if (!buildGraph(model_, g, error)) return {};
    g.prepare(sampleRate_, block_, model_.clock.tempo);
    g.transport().setLoop(0.0, 0.0, false);
    primeNoteTracks(g);
    const int idx = g.indexOf(source);
    const int chans = g.outputChannels(idx);
    if (idx < 0 || chans <= 0) { error = source + " has no audio output"; return {}; }

    std::vector<std::vector<float>> pcm((size_t) chans);
    const double guard = songEndBeat() + (toBeat - fromBeat) + 64.0;
    while (g.transport().beats() < toBeat && g.transport().beats() < guard) {
        const double b0 = g.transport().beats();
        g.processBlock(block_);
        const double b1 = g.transport().beats();
        if (b1 <= b0) break;
        const double perSample = (b1 - b0) / block_;
        for (int n = 0; n < block_; ++n) {
            const double beat = b0 + perSample * n;
            if (beat < fromBeat || beat >= toBeat) continue;
            for (int c = 0; c < chans; ++c)
                pcm[(size_t) c].push_back(g.outputData(idx, c)[n]);
        }
    }
    if (pcm[0].empty()) { error = "that range rendered nothing"; return {}; }

    const auto dir = record_.recordingsDir();
    dir.createDirectory();
    std::vector<std::string> existing;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        existing.push_back(f.getFileName().toStdString());
    const auto path = dir.getChildFile(juce::String(rec::nextTakeName(existing, node + "-consolidated")))
                          .getFullPathName().toStdString();
    if (!writeWav(path, pcm, sampleRate_)) { error = "could not write " + path; return {}; }

    std::vector<ConnectionModel> downstream;
    for (const auto& c : model_.connections)
        if (c.src == source) downstream.push_back(c);

    beginTransaction();
    pushUndo();
    const int nRows = (int) arrangeableNodes().size();
    const auto track = addOrganism("AudioTrack", spotBelowPatch());
    if (!track.empty()) {
        const int outs = std::max(1, outletsOf(track));
        for (const auto& c : downstream)
            connect(track, std::min(c.srcOutlet, outs - 1), c.dst, c.dstInlet);
        if (downstream.empty()) connectToMaster(track);
        clips().addAudio(track, (int) std::llround(fromBeat * Pattern::kTicksPerBeat),
                         std::max(1, (int) std::llround((toBeat - fromBeat)
                                                        * Pattern::kTicksPerBeat)),
                         path);
        bool noteTrack = false;
        if (const auto* cm = model_.byName(node))
            for (const auto& ch : cm->pattern.channels)
                if (ch.type == "note-events") noteTrack = true;
        bool hasMute = false;
        if (const auto* cm = model_.byName(node))
            for (const auto& d : schemaFor(cm->classRaw))
                if (d.name == "Mute") hasMute = true;
        if (noteTrack)    setTrackMuted(node, true);
        else if (hasMute) setParam(node, "Mute", 1.0);
        else              setBypass(node, true);
    }
    endTransaction();
    return track;
}

std::vector<std::string> EngineHost::noteTargets(const std::string& node) {
    std::vector<std::string> out;
    auto instrument = [&](const std::string& n) {
        return midiInletsOf(n) > 0 && graph_ != nullptr
               && dynamic_cast<ClipArrangement*>(graph_->find(n)) == nullptr;
    };
    for (const auto& c : model_.midiConnections)
        if (c.src == node && instrument(c.dst)
            && std::find(out.begin(), out.end(), c.dst) == out.end())
            out.push_back(c.dst);
    return out;
}

std::string EngineHost::printToTimeline(const std::string& node, std::string& error, int atTick,
                                        const std::string& preferredTarget) {
    const auto* cm = model_.byName(node);
    if (cm == nullptr || midiOutletsOf(node) <= 0) { error = node + " emits no notes"; return {}; }
    const auto targets = noteTargets(node);
    if (targets.empty()) { error = "cord " + node + " to an instrument first"; return {}; }
    std::string target = preferredTarget;
    if (std::find(targets.begin(), targets.end(), target) == targets.end()) target = targets.front();
    std::vector<int> ports;
    for (const auto& c : model_.midiConnections)
        if (c.src == node && c.dst == target) ports.push_back(c.srcOutlet);

    const int lengthTicks = cm->pattern.duration > 0 ? cm->pattern.duration
                                                     : 16 * Pattern::kTicksPerBeat;
    const double lengthBeats = (double) lengthTicks / Pattern::kTicksPerBeat;

    AudioGraph g;
    if (!buildGraph(model_, g, error)) return {};
    g.prepare(sampleRate_, block_, model_.clock.tempo);
    g.transport().setLoop(0.0, 0.0, false);
    const int idx = g.indexOf(node);
    if (idx < 0) { error = node + " is not in the graph"; return {}; }

    std::vector<TimedMidi> events;
    while (g.transport().beats() < lengthBeats + 1.0) {
        const double b0 = g.transport().beats();
        g.processBlock(block_);
        const double b1 = g.transport().beats();
        if (b1 <= b0) break;
        const double perSample = (b1 - b0) / block_;
        for (int port : ports) {
            const MidiEvent* ev = nullptr;
            const int n = g.nodeMidiOut(idx, port, ev);
            for (int i = 0; i < n; ++i) {
                const auto st = (unsigned char) (ev[i].data[0] & 0xF0);
                if (st != 0x90 && st != 0x80) continue;
                events.push_back({b0 + perSample * ev[i].sampleOffset,
                                  ev[i].data[0], ev[i].data[1], ev[i].data[2]});
            }
        }
    }
    std::vector<TimedMidi> remaining;
    const auto notes = assembleRecordedNotes(events, lengthTicks, 0, true,
                                             Pattern::kTicksPerBeat / 4, remaining, 0.0, false);
    if (notes.empty()) { error = node + " played nothing in that span"; return {}; }
    std::sort(events.begin(), events.end(), [](const TimedMidi& a, const TimedMidi& b) { return a.beat < b.beat; });

    const int bar = 4 * Pattern::kTicksPerBeat;
    if (atTick < 0) atTick = (int) (std::llround(positionBeats() * Pattern::kTicksPerBeat) / bar) * bar;
    beginTransaction();
    pushUndo();
    const int clip = clips().add(target, atTick, lengthTicks);
    if (clip >= 0) {
        clips().setNotes(target, clip, notes, lengthTicks);
        clips().rename(target, clip, node);
    }
    endTransaction();
    if (onArrangementChanged) juce::MessageManager::callAsync([cb = onArrangementChanged] { cb(); });
    return clip >= 0 ? target : std::string();
}

}
