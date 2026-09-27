// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstddef>
#include <vector>

#include "core/midi/ControlMode.h"
#include "core/midi/MidiSource.h"

namespace hum {

class MidiSourceFrame {
public:
    MidiSourceFrame() : values_(kSlots, -1), fresh_(kSlots, 0) {
        for (auto& d : ticks_) d.assign(kTickSlots, 0);
    }

    static std::size_t at(int lane, int source) {
        return (std::size_t) lane * kMidiSourceCount + (std::size_t) source;
    }

    int value(int lane, int source) const { return values_[at(lane, source)]; }
    bool fresh(int lane, int source) const { return fresh_[at(lane, source)] != 0; }
    const int* laneValues(int lane) const { return values_.data() + at(lane, 0); }

    int encoderTicks(EncoderFormat f, int lane, int cc) const {
        return isCcSource(cc) ? ticks_[(std::size_t) f][tickAt(lane, cc)] : 0;
    }

    void send(int source, int value, int port = kAnyMidiPort, int channel = kAnyMidiChannel) {
        if (!isMidiSource(source)) return;
        forEachLane(port, channel, [&](int lane) {
            values_[at(lane, source)] = value;
            markFresh(at(lane, source));
        });
    }

    void sendEncoder(int cc, int value7, int port = kAnyMidiPort, int channel = kAnyMidiChannel) {
        if (!isCcSource(cc)) return;
        forEachLane(port, channel, [&](int lane) {
            for (std::size_t f = 0; f < kEncoderFormats; ++f)
                ticks_[f][tickAt(lane, cc)] += decodeEncoder((EncoderFormat) f, value7);
            ticked_.push_back(tickAt(lane, cc));
        });
    }

    void clearFresh() {
        for (auto i : touched_) fresh_[i] = 0;
        touched_.clear();
        for (auto i : ticked_)
            for (auto& d : ticks_) d[i] = 0;
        ticked_.clear();
    }

private:
    static constexpr std::size_t kSlots = (std::size_t) kMidiLanes * kMidiSourceCount;
    static constexpr std::size_t kTickSlots = (std::size_t) kMidiLanes * kMidiCcCount;

    static std::size_t tickAt(int lane, int cc) {
        return (std::size_t) lane * kMidiCcCount + (std::size_t) cc;
    }

    template <typename Fn>
    static void forEachLane(int port, int channel, Fn fn) {
        const int ports[2] = {midiPortRow(port), kAnyMidiPortRow};
        const int channels[2] = {midiChannelSlot(channel), kAnyMidiChannel};
        const int portCount = ports[0] == ports[1] ? 1 : 2;
        const int channelCount = channels[0] == channels[1] ? 1 : 2;
        for (int p = 0; p < portCount; ++p)
            for (int c = 0; c < channelCount; ++c)
                fn(ports[p] * kMidiChannelSlots + channels[c]);
    }

    void markFresh(std::size_t i) {
        if (fresh_[i] == 0) touched_.push_back(i);
        fresh_[i] = 1;
    }

    std::vector<int> values_;
    std::vector<char> fresh_;
    std::array<std::vector<int>, kEncoderFormats> ticks_;
    std::vector<std::size_t> touched_;
    std::vector<std::size_t> ticked_;
};

}
