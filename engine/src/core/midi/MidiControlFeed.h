// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

#include "core/midi/MidiSource.h"
#include "core/midi/MidiSourceFrame.h"
#include "core/midi/SteadyController.h"

namespace hum {

struct MidiControlEvent {
    int source = 0;
    int value = 0;
    int port = kAnyMidiPort;
    int channel = kAnyMidiChannel;
    double atMs = 0.0;
};

class MidiControlInbox {
public:
    static constexpr std::size_t kCapacity = 8192;

    void push(const MidiControlEvent& e) {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (pending_.size() < kCapacity) pending_.push_back(e);
    }

    void drainInto(std::vector<MidiControlEvent>& out) {
        out.clear();
        const std::lock_guard<std::mutex> lock(mutex_);
        out.swap(pending_);
    }

private:
    std::mutex mutex_;
    std::vector<MidiControlEvent> pending_;
};

class MidiControlFeed {
public:
    static constexpr std::size_t kTrail = 128;

    MidiControlFeed() : steady_((std::size_t) kMidiLanes * kMidiCcCount) {}

    void beginTick() { frame_.clearFresh(); }

    void take(const MidiControlEvent& e) {
        if (!isMidiSource(e.source)) return;
        trail_.push_back(e);
        if (trail_.size() > kTrail) trail_.pop_front();
        if (!isCcSource(e.source)) {
            frame_.send(e.source, e.value, e.port, e.channel);
            return;
        }
        frame_.sendEncoder(e.source, e.value, e.port, e.channel);
        auto& steady = steady_[(std::size_t) midiLane(e.port, e.channel) * kMidiCcCount
                               + (std::size_t) e.source];
        if (steadycc::moved(steady, e.value)) frame_.send(e.source, steady.applied, e.port, e.channel);
    }

    const MidiSourceFrame& frame() const { return frame_; }
    const std::deque<MidiControlEvent>& trail() const { return trail_; }

private:
    MidiSourceFrame frame_;
    std::vector<steadycc::Steady> steady_;
    std::deque<MidiControlEvent> trail_;
};

}
