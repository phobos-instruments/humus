// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "core/midi/ControlMode.h"
#include "core/midi/MidiFormat.h"

namespace hum {

inline constexpr int kNoteSourceBase = 128;
inline constexpr int kNoteSourceEnd = kNoteSourceBase + 128;
inline constexpr int kPitchBendSource = kNoteSourceEnd;
inline constexpr int kPressureSource = kPitchBendSource + 1;
inline constexpr int kMidiSourceCount = kPressureSource + 1;
inline constexpr int kMidiCcCount = kNoteSourceBase;
inline constexpr int kPitchBendMax = 16383;

inline constexpr int kMidiPorts = 8;
inline constexpr int kAnyMidiPort = -1;
inline constexpr int kMidiPortRows = kMidiPorts + 1;
inline constexpr int kAnyMidiPortRow = kMidiPorts;
inline int midiPortRow(int port) {
    return port >= 0 && port < kMidiPorts ? port : kAnyMidiPortRow;
}

inline constexpr int kAnyMidiChannel = 0;
inline constexpr int kMidiChannelSlots = 17;
inline int midiChannelSlot(int channel) {
    return channel >= 1 && channel <= 16 ? channel : kAnyMidiChannel;
}

inline constexpr int kMidiLanes = kMidiPortRows * kMidiChannelSlots;
inline int midiLane(int port, int channel) {
    return midiPortRow(port) * kMidiChannelSlots + midiChannelSlot(channel);
}

inline constexpr int kHeldThreshold = 63;
inline bool isMidiSource(int id) { return id >= 0 && id < kMidiSourceCount; }
inline bool isCcSource(int id) { return id >= 0 && id < kMidiCcCount; }
inline bool isNoteSource(int id) { return id >= kNoteSourceBase && id < kNoteSourceEnd; }
inline bool canBeHeld(int id) { return isCcSource(id) || isNoteSource(id); }
inline int noteOfSource(int id) { return id - kNoteSourceBase; }
inline int sourceForNote(int note) { return kNoteSourceBase + note; }

inline MidiMessageType messageTypeOf(int id) {
    if (isNoteSource(id)) return MidiMessageType::Note;
    if (id == kPitchBendSource) return MidiMessageType::PitchBend;
    if (id == kPressureSource) return MidiMessageType::ChannelPressure;
    return MidiMessageType::ControlChange;
}

inline int sourceMaxValue(int id) { return id == kPitchBendSource ? kPitchBendMax : kMidiMax; }

inline int sourceNumber(int id) {
    if (isNoteSource(id)) return noteOfSource(id);
    if (isCcSource(id)) return id;
    return 0;
}

inline int sourceFromMessage(MidiMessageType type, int number) {
    switch (type) {
        case MidiMessageType::Note:
            return number >= 0 && number < 128 ? sourceForNote(number) : -1;
        case MidiMessageType::PitchBend: return kPitchBendSource;
        case MidiMessageType::ChannelPressure: return kPressureSource;
        case MidiMessageType::ControlChange: return isMidiSource(number) ? number : -1;
    }
    return -1;
}

inline std::string midiSourceLabel(int id) {
    switch (messageTypeOf(id)) {
        case MidiMessageType::Note: return "Note " + midiNoteName(noteOfSource(id));
        case MidiMessageType::PitchBend: return "Pitch bend";
        case MidiMessageType::ChannelPressure: return "Pressure";
        case MidiMessageType::ControlChange: break;
    }
    return "CC " + std::to_string(id);
}

inline std::vector<int> normalizedHeld(std::vector<int> held, int cc) {
    held.erase(std::remove_if(held.begin(), held.end(),
                              [cc](int h) { return h == cc || !canBeHeld(h); }),
               held.end());
    std::sort(held.begin(), held.end());
    held.erase(std::unique(held.begin(), held.end()), held.end());
    return held;
}

struct MidiSource {
    int cc = 0;
    std::vector<int> held;
    int port = kAnyMidiPort;
    int channel = kAnyMidiChannel;

    MidiSource() = default;
    MidiSource(int c, std::vector<int> h = {}, int p = kAnyMidiPort, int ch = kAnyMidiChannel)
        : cc(c), held(normalizedHeld(std::move(h), c)), port(p), channel(midiChannelSlot(ch)) {}

    bool isChord() const { return !held.empty(); }
    bool anyPort() const { return port < 0 || port >= kMidiPorts; }
    bool anyChannel() const { return channel == kAnyMidiChannel; }
    int lane() const { return midiLane(port, channel); }
    bool operator==(const MidiSource& o) const {
        return cc == o.cc && held == o.held && lane() == o.lane();
    }
    bool operator!=(const MidiSource& o) const { return !(*this == o); }
};

inline std::string midiChannelLabel(int channel) {
    return midiChannelSlot(channel) == kAnyMidiChannel ? std::string()
                                                       : "ch " + std::to_string(channel);
}

inline std::string midiSourceLabel(const MidiSource& s) {
    std::string out = midiSourceLabel(s.cc);
    if (!s.anyChannel()) out += " (" + midiChannelLabel(s.channel) + ")";
    for (size_t i = 0; i < s.held.size(); ++i)
        out += (i == 0 ? " while holding " : " + ") + midiSourceLabel(s.held[i]);
    return out;
}

}
