// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum::midisong {

inline constexpr int kChannels = 16;
inline constexpr int kDrumChannel = 9;

enum class Kind : uint8_t { NoteOn, NoteOff, Program, Controller, PitchBend, AllNotesOff };

struct Event {
    double beat = 0.0;
    Kind kind = Kind::NoteOn;
    uint8_t channel = 0;
    uint8_t a = 0;
    uint8_t b = 0;
};

struct Lyric {
    double beat = 0.0;
    std::string text;
};

struct TempoPoint {
    double beat = 0.0;
    double bpm = 120.0;
};

struct Song {
    std::vector<Event> events;
    std::vector<Lyric> lyrics;
    std::vector<TempoPoint> tempo;
    std::vector<uint8_t> program = std::vector<uint8_t>(kChannels, 0);
    double lengthBeats = 0.0;
    std::string name;

    double bpmAt(double beat) const;
    double secondsForBeats(double beat) const;
    double beatsForSeconds(double seconds) const;
};

bool looksLikeMidi(const void* data, size_t n);
bool looksLikeMus(const void* data, size_t n);

bool parseMidi(const void* data, size_t n, Song& out);
bool parseMus(const void* data, size_t n, Song& out);

bool parseSong(const void* data, size_t n, Song& out);
bool loadSong(const juce::File& file, Song& out);

const char* wildcard();

}
