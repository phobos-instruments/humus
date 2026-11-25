// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstdint>

#include <clap/events.h>
#include <clap/ext/note-ports.h>

#include "core/plugins/clap/ClapEvents.h"

namespace hum::claphost {

enum class NoteDialect { RawMidi, ClapNotes, ClapExpressive };

NoteDialect dialectFor(uint32_t supported, uint32_t preferred);

class NoteTranslator {
public:
    static constexpr int kChannels = 16, kKeys = 128;
    static constexpr double kDefaultBendRange = 2.0, kMpeMemberBendRange = 48.0;

    NoteTranslator() { reset(); }
    void reset();
    bool push(EventList& out, const uint8_t* data, int size, uint32_t time, uint16_t port, NoteDialect dialect);
    double bendRange(int channel) const { return range_[(size_t) (channel & 0x0f)]; }
    int32_t noteId(int channel, int key) const { return ids_[(size_t) (channel & 0x0f)][(size_t) (key & 0x7f)]; }

private:
    struct Target {
        EventList& out;
        uint32_t time;
        uint16_t port;
    };

    bool note(const Target& t, bool on, int channel, int key, int velocity, bool expressive);
    void controller(const Target& t, int channel, int cc, int value);
    void dataEntry(int channel, int msb, int lsb);
    void declareZone(int master, int members);
    void express(const Target& t, int channel, int key, clap_note_expression what, double value);
    void expressChannel(const Target& t, int channel, clap_note_expression what);
    double valueOf(int channel, clap_note_expression what) const;
    int masterOf(int channel) const;
    bool inZone(int channel, int master) const;

    std::array<double, kChannels> range_{}, bend_{}, pressure_{}, brightness_{};
    std::array<int, kChannels> rpnMsb_{}, rpnLsb_{}, dataMsb_{};
    std::array<std::array<int32_t, kKeys>, kChannels> ids_{};
    int lowerMembers_ = 0, upperMembers_ = 0;
    int32_t nextId_ = 0;
};

}
