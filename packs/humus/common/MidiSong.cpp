// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/MidiSong.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/dsp/DspMath.h"

namespace hum::midisong {

namespace {

constexpr double kDefaultBpm = 120.0;

void sortByBeat(std::vector<Event>& events) {
    std::stable_sort(events.begin(), events.end(),
                     [](const Event& a, const Event& b) { return a.beat < b.beat; });
}

void noteOffEverything(Song& song) {
    if (song.events.empty()) return;
    song.lengthBeats = song.events.back().beat;
    for (const auto& e : song.events) song.lengthBeats = std::max(song.lengthBeats, e.beat);
}

}

double Song::bpmAt(double beat) const {
    double bpm = kDefaultBpm;
    for (const auto& t : tempo) {
        if (t.beat > beat) break;
        bpm = t.bpm;
    }
    return bpm;
}

double Song::secondsForBeats(double beat) const {
    if (tempo.empty()) return beat * kSecondsPerMinute / kDefaultBpm;
    double seconds = 0.0, at = 0.0, bpm = tempo.front().bpm;
    for (const auto& t : tempo) {
        if (t.beat >= beat) break;
        seconds += (t.beat - at) * kSecondsPerMinute / bpm;
        at = t.beat;
        bpm = t.bpm;
    }
    return seconds + (beat - at) * kSecondsPerMinute / bpm;
}

double Song::beatsForSeconds(double seconds) const {
    if (tempo.empty()) return seconds * kDefaultBpm / kSecondsPerMinute;
    double at = 0.0, spent = 0.0, bpm = tempo.front().bpm;
    for (const auto& t : tempo) {
        const double span = (t.beat - at) * kSecondsPerMinute / bpm;
        if (spent + span >= seconds) break;
        spent += span;
        at = t.beat;
        bpm = t.bpm;
    }
    return at + (seconds - spent) * bpm / kSecondsPerMinute;
}

bool looksLikeMidi(const void* data, size_t n) {
    return n >= 4 && std::memcmp(data, "MThd", 4) == 0;
}

bool looksLikeMus(const void* data, size_t n) {
    return n >= 4 && std::memcmp(data, "MUS\x1a", 4) == 0;
}

namespace {

void addLyric(Song& song, double beat, const juce::String& text) {
    if (text.isEmpty()) return;
    if (text.startsWith("@")) return;
    song.lyrics.push_back({beat, text.toStdString()});
}

}

bool parseMidi(const void* data, size_t n, Song& out) {
    if (!looksLikeMidi(data, n)) return false;
    juce::MemoryInputStream in(data, n, false);
    juce::MidiFile file;
    if (!file.readFrom(in, false)) return false;

    const short format = file.getTimeFormat();
    if (format <= 0) return false;
    const double ppq = (double) format;

    out.events.clear();
    out.lyrics.clear();
    out.tempo.clear();
    out.program.assign(kChannels, 0);

    bool programSeen[kChannels] = {};
    for (int t = 0; t < file.getNumTracks(); ++t) {
        const auto* seq = file.getTrack(t);
        if (seq == nullptr) continue;
        for (int i = 0; i < seq->getNumEvents(); ++i) {
            const auto& m = seq->getEventPointer(i)->message;
            const double beat = m.getTimeStamp() / ppq;
            if (m.isTempoMetaEvent()) {
                const double spq = m.getTempoSecondsPerQuarterNote();
                if (spq > 0.0) out.tempo.push_back({beat, kSecondsPerMinute / spq});
                continue;
            }
            if (m.isTrackNameEvent() && out.name.empty() && t == 0) {
                out.name = m.getTextFromTextMetaEvent().toStdString();
                continue;
            }
            if (m.isTextMetaEvent()) {
                const int type = m.getMetaEventType();
                if (type == 0x05 || type == 0x01)
                    addLyric(out, beat, m.getTextFromTextMetaEvent());
                continue;
            }
            if (!m.getChannel()) continue;
            const auto ch = (uint8_t) (m.getChannel() - 1);
            if (m.isNoteOn())
                out.events.push_back({beat, Kind::NoteOn, ch, (uint8_t) m.getNoteNumber(),
                                      (uint8_t) m.getVelocity()});
            else if (m.isNoteOff())
                out.events.push_back({beat, Kind::NoteOff, ch, (uint8_t) m.getNoteNumber(), 0});
            else if (m.isProgramChange()) {
                const auto p = (uint8_t) juce::jlimit(0, kMidiMax, m.getProgramChangeNumber());
                out.events.push_back({beat, Kind::Program, ch, p, 0});
                if (!programSeen[ch]) { out.program[ch] = p; programSeen[ch] = true; }
            } else if (m.isController())
                out.events.push_back({beat, Kind::Controller, ch,
                                      (uint8_t) m.getControllerNumber(),
                                      (uint8_t) m.getControllerValue()});
            else if (m.isPitchWheel()) {
                const int w = juce::jlimit(0, 16383, m.getPitchWheelValue());
                out.events.push_back({beat, Kind::PitchBend, ch, (uint8_t) (w & 0x7f),
                                      (uint8_t) ((w >> 7) & 0x7f)});
            } else if (m.isAllNotesOff())
                out.events.push_back({beat, Kind::AllNotesOff, ch, 0, 0});
        }
    }
    sortByBeat(out.events);
    std::stable_sort(out.tempo.begin(), out.tempo.end(),
                     [](const TempoPoint& a, const TempoPoint& b) { return a.beat < b.beat; });
    std::stable_sort(out.lyrics.begin(), out.lyrics.end(),
                     [](const Lyric& a, const Lyric& b) { return a.beat < b.beat; });
    if (out.tempo.empty()) out.tempo.push_back({0.0, kDefaultBpm});
    noteOffEverything(out);
    return !out.events.empty();
}

bool parseSong(const void* data, size_t n, Song& out) {
    if (looksLikeMus(data, n)) return parseMus(data, n, out);
    return parseMidi(data, n, out);
}

bool loadSong(const juce::File& file, Song& out) {
    if (!file.existsAsFile()) return false;
    juce::MemoryBlock block;
    if (!file.loadFileAsData(block)) return false;
    if (!parseSong(block.getData(), block.getSize(), out)) return false;
    if (out.name.empty()) out.name = file.getFileNameWithoutExtension().toStdString();
    return true;
}

const char* wildcard() { return "*.mid;*.midi;*.kar;*.mus"; }

}
