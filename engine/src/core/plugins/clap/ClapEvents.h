// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include <clap/events.h>

namespace hum::claphost {

class EventList {
public:
    explicit EventList(size_t maxEvents = 4096, size_t bytesPerEvent = 64);

    void clear();
    bool push(const clap_event_header_t& event);
    uint32_t size() const { return (uint32_t) count_; }
    const clap_event_header_t* at(uint32_t index) const;

    const clap_input_events_t* input() const { return &input_; }
    const clap_output_events_t* output() const { return &output_; }

    EventList(const EventList&) = delete;
    EventList& operator=(const EventList&) = delete;

private:
    std::vector<uint64_t> arena_;
    std::vector<size_t> offsets_;
    size_t used_ = 0, count_ = 0;
    clap_input_events_t input_{};
    clap_output_events_t output_{};
};

int midiMessageSize(uint8_t status);
bool pushMidi(EventList& list, const uint8_t* data, int size, uint32_t time, uint16_t port, bool asClapNotes);
int toMidi(const clap_event_header_t& event, uint8_t out[3]);

}
