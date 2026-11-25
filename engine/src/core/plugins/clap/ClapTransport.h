// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>

#include <clap/events.h>
#include <clap/fixedpoint.h>

#include "hum/dsp/DspMath.h"

namespace hum::claphost {

struct TransportState {
    double tempo = 120.0;
    double beats = 0.0;
    double seconds = 0.0;
    bool playing = false;
    bool looping = false;
    double loopStartBeats = 0.0, loopEndBeats = 0.0;
    int numerator = 4, denominator = 4;
};

inline clap_beattime beatTime(double beats) { return (clap_beattime) std::llround(beats * (double) CLAP_BEATTIME_FACTOR); }
inline clap_sectime secTime(double seconds) { return (clap_sectime) std::llround(seconds * (double) CLAP_SECTIME_FACTOR); }

inline clap_event_transport_t makeTransport(const TransportState& t) {
    clap_event_transport_t ev{};
    ev.header = {sizeof(ev), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
    ev.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | CLAP_TRANSPORT_HAS_SECONDS_TIMELINE
               | CLAP_TRANSPORT_HAS_TIME_SIGNATURE;
    if (t.playing) ev.flags |= CLAP_TRANSPORT_IS_PLAYING;
    if (t.looping) ev.flags |= CLAP_TRANSPORT_IS_LOOP_ACTIVE;
    ev.tempo = t.tempo;
    ev.song_pos_beats = beatTime(t.beats);
    ev.song_pos_seconds = secTime(t.seconds);
    ev.loop_start_beats = beatTime(t.loopStartBeats);
    ev.loop_end_beats = beatTime(t.loopEndBeats);
    if (t.tempo > 0.0) {
        ev.loop_start_seconds = secTime(t.loopStartBeats * kSecondsPerMinute / t.tempo);
        ev.loop_end_seconds = secTime(t.loopEndBeats * kSecondsPerMinute / t.tempo);
    }
    const int num = t.numerator > 0 ? t.numerator : 4;
    const int den = t.denominator > 0 ? t.denominator : 4;
    const double beatsPerBar = num * 4.0 / den;
    const double bar = std::floor(t.beats / beatsPerBar);
    ev.bar_number = (int32_t) bar;
    ev.bar_start = beatTime(bar * beatsPerBar);
    ev.tsig_num = (uint16_t) num;
    ev.tsig_denom = (uint16_t) den;
    return ev;
}

}
