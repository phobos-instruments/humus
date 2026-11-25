// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/clap/ClapNotes.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum::claphost {

namespace {
constexpr int kNoRpn = kSevenBitMax;
constexpr int kBendCentre = 8192;
constexpr int kRpnMsbCc = 101, kRpnLsbCc = 100, kDataMsbCc = 6, kDataLsbCc = 38, kTimbreCc = 74;
constexpr int kBendRangeRpn = 0, kMpeConfigRpn = 6;
constexpr int kLowerMaster = 0, kUpperMaster = 15;
constexpr double kCentsPerSemitone = 100.0;
}

NoteDialect dialectFor(uint32_t supported, uint32_t preferred) {
    if ((supported & CLAP_NOTE_DIALECT_MIDI_MPE) != 0) return NoteDialect::RawMidi;
    if ((supported & CLAP_NOTE_DIALECT_CLAP) == 0) return NoteDialect::RawMidi;
    if ((supported & CLAP_NOTE_DIALECT_MIDI) == 0) return NoteDialect::ClapExpressive;
    return preferred == CLAP_NOTE_DIALECT_CLAP ? NoteDialect::ClapNotes : NoteDialect::RawMidi;
}

void NoteTranslator::reset() {
    range_.fill(kDefaultBendRange);
    bend_.fill(0.0);
    pressure_.fill(0.0);
    brightness_.fill(0.5);
    rpnMsb_.fill(kNoRpn);
    rpnLsb_.fill(kNoRpn);
    dataMsb_.fill(0);
    for (auto& keys : ids_) keys.fill(-1);
    lowerMembers_ = upperMembers_ = 0;
}

bool NoteTranslator::push(EventList& out, const uint8_t* data, int size, uint32_t time, uint16_t port,
                          NoteDialect dialect) {
    if (size < 1 || data == nullptr) return false;
    if (data[0] == 0xf0) {
        clap_event_midi_sysex_t sysex{};
        sysex.header = {sizeof(sysex), time, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI_SYSEX, 0};
        sysex.port_index = port;
        sysex.buffer = data;
        sysex.size = (uint32_t) size;
        return out.push(sysex.header);
    }
    if (midiMessageSize(data[0]) != size) return false;
    if (dialect == NoteDialect::RawMidi) return pushMidi(out, data, size, time, port, false);
    const Target t{out, time, port};
    const int kind = data[0] & 0xf0, channel = data[0] & 0x0f;
    const bool expressive = dialect == NoteDialect::ClapExpressive;
    if ((kind == 0x90 || kind == 0x80) && size == 3)
        return note(t, kind == 0x90 && data[2] > 0, channel, data[1], data[2], expressive);
    if (kind == 0xb0) controller(t, channel, data[1], data[2]);
    if (!expressive) return pushMidi(out, data, size, time, port, false);
    if (kind == 0xe0) {
        bend_[(size_t) channel] = (((data[2] << 7) | data[1]) - kBendCentre) / (double) kBendCentre * range_[(size_t) channel];
        expressChannel(t, channel, CLAP_NOTE_EXPRESSION_TUNING);
    } else if (kind == 0xd0) {
        pressure_[(size_t) channel] = data[1] / (double) kMidiMax;
        expressChannel(t, channel, CLAP_NOTE_EXPRESSION_PRESSURE);
    } else if (kind == 0xa0) {
        express(t, channel, data[1], CLAP_NOTE_EXPRESSION_PRESSURE, data[2] / (double) kMidiMax);
    } else if (kind == 0xb0 && data[1] == kTimbreCc) {
        brightness_[(size_t) channel] = data[2] / (double) kMidiMax;
        expressChannel(t, channel, CLAP_NOTE_EXPRESSION_BRIGHTNESS);
    } else {
        return pushMidi(out, data, size, time, port, false);
    }
    return true;
}

bool NoteTranslator::note(const Target& t, bool on, int channel, int key, int velocity, bool expressive) {
    auto& id = ids_[(size_t) channel][(size_t) key];
    clap_event_note_t ev{};
    ev.header = {sizeof(ev), t.time, CLAP_CORE_EVENT_SPACE_ID, (uint16_t) (on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF), 0};
    if (on) {
        id = nextId_;
        nextId_ = (nextId_ + 1) & 0x7fffffff;
    }
    ev.note_id = id;
    ev.port_index = (int16_t) t.port;
    ev.channel = (int16_t) channel;
    ev.key = (int16_t) key;
    ev.velocity = velocity / (double) kMidiMax;
    const bool pushed = t.out.push(ev.header);
    if (!on) id = -1;
    if (on && expressive) {
        if (valueOf(channel, CLAP_NOTE_EXPRESSION_TUNING) != 0.0)
            express(t, channel, key, CLAP_NOTE_EXPRESSION_TUNING, valueOf(channel, CLAP_NOTE_EXPRESSION_TUNING));
        if (pressure_[(size_t) channel] != 0.0)
            express(t, channel, key, CLAP_NOTE_EXPRESSION_PRESSURE, pressure_[(size_t) channel]);
    }
    return pushed;
}

void NoteTranslator::controller(const Target&, int channel, int cc, int value) {
    const auto c = (size_t) channel;
    if (cc == kRpnMsbCc) rpnMsb_[c] = value;
    else if (cc == kRpnLsbCc) rpnLsb_[c] = value;
    else if (cc == kDataMsbCc) {
        dataMsb_[c] = value;
        dataEntry(channel, value, -1);
    } else if (cc == kDataLsbCc) {
        dataEntry(channel, dataMsb_[c], value);
    }
}

void NoteTranslator::dataEntry(int channel, int msb, int lsb) {
    const auto c = (size_t) channel;
    if (rpnMsb_[c] != 0) return;
    if (rpnLsb_[c] == kBendRangeRpn) range_[c] = msb + (lsb > 0 ? lsb / kCentsPerSemitone : 0.0);
    else if (rpnLsb_[c] == kMpeConfigRpn && lsb < 0) declareZone(channel, msb);
}

void NoteTranslator::declareZone(int master, int members) {
    if (master != kLowerMaster && master != kUpperMaster) return;
    members = std::clamp(members, 0, kChannels - 1);
    (master == kLowerMaster ? lowerMembers_ : upperMembers_) = members;
    range_[(size_t) master] = kDefaultBendRange;
    for (int ch = 0; ch < kChannels; ++ch)
        if (ch != master && inZone(ch, master)) range_[(size_t) ch] = kMpeMemberBendRange;
}

bool NoteTranslator::inZone(int channel, int master) const {
    if (master == kLowerMaster) return channel >= 1 && channel <= lowerMembers_;
    if (master == kUpperMaster) return channel <= kUpperMaster - 1 && channel >= kUpperMaster - upperMembers_;
    return false;
}

int NoteTranslator::masterOf(int channel) const {
    if (inZone(channel, kLowerMaster)) return kLowerMaster;
    if (inZone(channel, kUpperMaster)) return kUpperMaster;
    return -1;
}

double NoteTranslator::valueOf(int channel, clap_note_expression what) const {
    const auto c = (size_t) channel;
    if (what == CLAP_NOTE_EXPRESSION_PRESSURE) return pressure_[c];
    if (what == CLAP_NOTE_EXPRESSION_BRIGHTNESS) return brightness_[c];
    const int master = masterOf(channel);
    return bend_[c] + (master >= 0 ? bend_[(size_t) master] : 0.0);
}

void NoteTranslator::express(const Target& t, int channel, int key, clap_note_expression what, double value) {
    const int32_t id = ids_[(size_t) channel][(size_t) key];
    if (id < 0) return;
    clap_event_note_expression_t ev{};
    ev.header = {sizeof(ev), t.time, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_NOTE_EXPRESSION, 0};
    ev.expression_id = what;
    ev.note_id = id;
    ev.port_index = (int16_t) t.port;
    ev.channel = (int16_t) channel;
    ev.key = (int16_t) key;
    ev.value = value;
    t.out.push(ev.header);
}

void NoteTranslator::expressChannel(const Target& t, int channel, clap_note_expression what) {
    const bool master = (channel == kLowerMaster && lowerMembers_ > 0) || (channel == kUpperMaster && upperMembers_ > 0);
    for (int ch = 0; ch < kChannels; ++ch) {
        if (ch != channel && !(master && inZone(ch, channel))) continue;
        for (int key = 0; key < kKeys; ++key)
            if (ids_[(size_t) ch][(size_t) key] >= 0) express(t, ch, key, what, valueOf(ch, what));
    }
}

}
