// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/clap/ClapEvents.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum::claphost {

namespace {
constexpr size_t kWord = sizeof(uint64_t);
size_t words(size_t bytes) { return (bytes + kWord - 1) / kWord; }
}

EventList::EventList(size_t maxEvents, size_t bytesPerEvent)
    : arena_(words(maxEvents * bytesPerEvent)), offsets_(maxEvents) {
    input_.ctx = this;
    input_.size = [](const clap_input_events_t* l) { return static_cast<const EventList*>(l->ctx)->size(); };
    input_.get = [](const clap_input_events_t* l, uint32_t i) { return static_cast<const EventList*>(l->ctx)->at(i); };
    output_.ctx = this;
    output_.try_push = [](const clap_output_events_t* l, const clap_event_header_t* e) {
        return e != nullptr && static_cast<EventList*>(l->ctx)->push(*e);
    };
}

void EventList::clear() {
    used_ = 0;
    count_ = 0;
}

bool EventList::push(const clap_event_header_t& event) {
    const size_t need = words(std::max<size_t>(event.size, sizeof(clap_event_header_t)));
    if (count_ >= offsets_.size() || used_ + need > arena_.size()) return false;
    std::memcpy(arena_.data() + used_, &event, event.size);
    size_t slot = count_;
    while (slot > 0 && at((uint32_t) (slot - 1))->time > event.time) {
        offsets_[slot] = offsets_[slot - 1];
        --slot;
    }
    offsets_[slot] = used_;
    used_ += need;
    ++count_;
    return true;
}

const clap_event_header_t* EventList::at(uint32_t index) const {
    if (index >= count_) return nullptr;
    return reinterpret_cast<const clap_event_header_t*>(arena_.data() + offsets_[index]);
}

int midiMessageSize(uint8_t status) {
    if (status < 0x80) return 0;
    if (status < 0xf0) return (status & 0xe0) == 0xc0 ? 2 : 3;
    if (status == 0xf1 || status == 0xf3) return 2;
    if (status == 0xf2) return 3;
    if (status == 0xf6 || status >= 0xf8) return 1;
    return 0;
}

bool pushMidi(EventList& list, const uint8_t* data, int size, uint32_t time, uint16_t port, bool asClapNotes) {
    if (size < 1 || size > 3 || midiMessageSize(data[0]) != size) return false;
    const int kind = data[0] & 0xf0;
    if (asClapNotes && size == 3 && (kind == 0x90 || kind == 0x80)) {
        clap_event_note_t note{};
        const bool on = kind == 0x90 && data[2] > 0;
        note.header = {sizeof(note), time, CLAP_CORE_EVENT_SPACE_ID,
                       (uint16_t) (on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF), 0};
        note.note_id = -1;
        note.port_index = (int16_t) port;
        note.channel = (int16_t) (data[0] & 0x0f);
        note.key = (int16_t) data[1];
        note.velocity = data[2] / (double) kMidiMax;
        return list.push(note.header);
    }
    clap_event_midi_t midi{};
    midi.header = {sizeof(midi), time, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI, 0};
    midi.port_index = port;
    for (int i = 0; i < size; ++i) midi.data[i] = data[i];
    return list.push(midi.header);
}

int toMidi(const clap_event_header_t& event, uint8_t out[3]) {
    if (event.space_id != CLAP_CORE_EVENT_SPACE_ID) return 0;
    if (event.type == CLAP_EVENT_MIDI) {
        const auto& midi = reinterpret_cast<const clap_event_midi_t&>(event);
        const int size = midiMessageSize(midi.data[0]);
        for (int i = 0; i < size; ++i) out[i] = midi.data[i];
        return size;
    }
    if (event.type == CLAP_EVENT_NOTE_ON || event.type == CLAP_EVENT_NOTE_OFF) {
        const auto& note = reinterpret_cast<const clap_event_note_t&>(event);
        if (note.key < 0 || note.key > kMidiMax) return 0;
        const int channel = std::clamp<int>(note.channel, 0, 15);
        const int velocity = std::clamp((int) std::lround(note.velocity * kMidiMax), 0, kMidiMax);
        out[0] = (uint8_t) ((event.type == CLAP_EVENT_NOTE_ON ? 0x90 : 0x80) | channel);
        out[1] = (uint8_t) note.key;
        out[2] = (uint8_t) (event.type == CLAP_EVENT_NOTE_ON ? std::max(1, velocity) : velocity);
        return 3;
    }
    return 0;
}

}
