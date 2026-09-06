#include "io/MidiExport.h"

#include <algorithm>
#include <cmath>

#include "hum/NoteSchedule.h"

#include "hum/dsp/DspMath.h"

namespace hum::midiexport {

namespace {

constexpr int kWindowBeats = 4;
constexpr int kMaxEdgesPerWindow = 4096;

bool carriesNotes(const OrganismModel& cm) {
    for (const auto& ch : cm.pattern.channels)
        if (ch.type == "note-events") return true;
    return false;
}

double lastBeatOf(const OrganismModel& cm) {
    double last = 0.0;
    for (const auto& ch : cm.pattern.channels) {
        if (ch.type != "note-events") continue;
        const int end = ch.startTick < 0 ? 4 * 4 * Pattern::kTicksPerBeat
                                         : ch.startTick + std::max(0, ch.lengthTicks);
        last = std::max(last, (double) end / Pattern::kTicksPerBeat);
    }
    return last;
}

void collect(const OrganismModel& cm, const Range& range, juce::MidiMessageSequence& seq,
             int channel) {
    const auto voices = noteschedule::prepare(cm.pattern, 4 * 4 * Pattern::kTicksPerBeat);
    if (voices.empty()) return;
    std::vector<noteschedule::Edge> edges((size_t) kMaxEdgesPerWindow);
    for (double at = range.fromBeat; at < range.toBeat; at += kWindowBeats) {
        const double until = std::min(at + kWindowBeats, range.toBeat);
        const int ticks = std::max(1, (int) std::llround((until - at) * Pattern::kTicksPerBeat));
        const int n = noteschedule::window(voices, at, until, 0, ticks,
                                           (double) Pattern::kTicksPerBeat, 1.0, edges.data(),
                                           kMaxEdgesPerWindow, 0, true);
        for (int i = 0; i < n; ++i) {
            const auto& e = edges[(size_t) i];
            const double tick = (at - range.fromBeat) * Pattern::kTicksPerBeat + e.offset;
            if (e.cc >= 0) {
                seq.addEvent(juce::MidiMessage::controllerEvent(
                                 channel, std::clamp(e.cc, 0, kMidiMax),
                                 std::clamp(e.vel, 0, kMidiMax)),
                             tick);
                continue;
            }
            const int pitch = std::clamp(e.pitch, 0, kMidiMax);
            seq.addEvent(e.on ? juce::MidiMessage::noteOn(channel, pitch,
                                                          (juce::uint8) std::clamp(e.vel, 1,
                                                                                   kMidiMax))
                              : juce::MidiMessage::noteOff(channel, pitch),
                         tick);
        }
    }
    seq.updateMatchedPairs();
}

}

std::vector<std::string> notedNodes(const PatchDocumentModel& model) {
    std::vector<std::string> out;
    for (const auto& cm : model.organisms)
        if (carriesNotes(cm)) out.push_back(cm.name);
    return out;
}

bool write(const PatchDocumentModel& model, const juce::File& out, const Range& range) {
    Range span = range;
    if (span.toBeat <= span.fromBeat) {
        span.fromBeat = 0.0;
        for (const auto& cm : model.organisms) span.toBeat = std::max(span.toBeat, lastBeatOf(cm));
    }
    if (span.toBeat <= span.fromBeat) return false;

    juce::MidiFile file;
    file.setTicksPerQuarterNote(Pattern::kTicksPerBeat);

    const double bpm = model.clock.tempo > 0.0 ? model.clock.tempo : 120.0;
    juce::MidiMessageSequence conductor;
    conductor.addEvent(juce::MidiMessage::tempoMetaEvent(
                           (int) std::llround(kSecondsPerMinute * 1.0e6 / bpm)),
                       0.0);
    conductor.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4), 0.0);
    file.addTrack(conductor);

    int channel = 1, written = 0;
    for (const auto& cm : model.organisms) {
        if (!carriesNotes(cm)) continue;
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::textMetaEvent(3, juce::String(cm.name)), 0.0);
        collect(cm, span, seq, channel);
        if (seq.getNumEvents() <= 1) continue;
        file.addTrack(seq);
        ++written;
        if (++channel > 16) channel = 1;
    }
    if (written == 0) return false;

    out.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream(out.createOutputStream());
    if (stream == nullptr) return false;
    const bool ok = file.writeTo(*stream);
    stream->flush();
    return ok;
}

}
