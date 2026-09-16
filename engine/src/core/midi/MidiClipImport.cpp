// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/midi/MidiClipImport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/Pattern.h"
#include "hum/dsp/DspMath.h"

namespace hum::midiclip {

namespace {

int toPatternTicks(double midiTicks, double ppq) {
    return (int) std::llround(midiTicks * (double) Pattern::kTicksPerBeat / ppq);
}

}

Imported importMidi(const std::uint8_t* data, std::size_t size) {
    Imported out;
    juce::MemoryInputStream in(data, size, false);
    juce::MidiFile file;
    if (!file.readFrom(in)) return out;
    const short format = file.getTimeFormat();
    if (format <= 0) return out;
    const double ppq = (double) format;

    for (int t = 0; t < file.getNumTracks(); ++t) {
        juce::MidiMessageSequence seq(*file.getTrack(t));
        seq.updateMatchedPairs();
        for (int i = 0; i < seq.getNumEvents(); ++i) {
            const auto* e = seq.getEventPointer(i);
            if (!e->message.isNoteOn()) continue;
            const double on = e->message.getTimeStamp();
            const double off = e->noteOffObject != nullptr
                                   ? e->noteOffObject->message.getTimeStamp() : on;
            NoteEvent n;
            n.tick = std::max(0, toPatternTicks(on, ppq));
            n.lengthTicks = std::max(1, toPatternTicks(std::max(off - on, 0.0), ppq));
            n.pitch = std::clamp(e->message.getNoteNumber(), 0, kMidiMax);
            n.velocity = std::clamp((int) e->message.getVelocity(), 1, kMidiMax);
            out.notes.push_back(n);
        }
    }
    if (out.notes.empty()) return {};

    std::sort(out.notes.begin(), out.notes.end(), [](const NoteEvent& a, const NoteEvent& b) {
        return a.tick != b.tick ? a.tick < b.tick : a.pitch < b.pitch;
    });
    int end = 0;
    for (const auto& n : out.notes) end = std::max(end, n.tick + n.lengthTicks);
    const int beat = Pattern::kTicksPerBeat;
    out.lengthTicks = std::max(beat, (end + beat - 1) / beat * beat);
    return out;
}

}
