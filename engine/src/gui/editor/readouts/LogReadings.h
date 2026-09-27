// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <deque>
#include <string>
#include <utility>

#include "core/midi/MidiFormat.h"
#include "gui/editor/LiveControls.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Osc.h"

namespace hum::readout {

class EventLog {
public:
    static constexpr size_t kMaxLines = 400;

    struct Line {
        std::string text;
        bool accent = false;
    };

    void push(std::string text, bool accent) { lines_.push_back({std::move(text), accent}); }
    void clear() { lines_.clear(); }
    void trim() {
        while (lines_.size() > kMaxLines) lines_.pop_front();
    }
    const std::deque<Line>& lines() const { return lines_; }

private:
    std::deque<Line> lines_;
};

class MidiLog {
public:
    MidiLog(std::string sourceParam, std::string channelParam)
        : sourceParam_(std::move(sourceParam)), channelParam_(std::move(channelParam)) {}

    bool drain(ModelHost& host, const std::string& organism, bool paused, EventLog& log) {
        auto* src = live::source<MidiLogSource>(host, organism);
        if (src == nullptr || src->logGeneration() == lastGen_) return false;
        lastGen_ = src->logGeneration();

        MidiLogSource::Logged events[256];
        const int count = src->consumeLog(events, 256);
        if (count == 0 || paused) return false;

        const int source = (int) host.liveParamValue(organism, sourceParam_);
        const int chan = (int) host.liveParamValue(organism, channelParam_);
        bool pushed = false;
        for (int i = 0; i < count; ++i) {
            const auto& e = events[i];
            const bool live = e.origin == 0;
            if (source == 1 && live) continue;
            if (source == 2 && !live) continue;
            const int evChan = midiEventChannel(e.event.data, e.event.size);
            if (chan > 0 && evChan > 0 && evChan != chan) continue;
            const std::string tag = !live ? "cord" : e.port >= 0 ? "in" + std::to_string(e.port + 1) : "kbd";
            log.push((tag + "    ").substr(0, 5) + formatMidiEvent(e.event.data, e.event.size), live);
            pushed = true;
        }
        return pushed;
    }

private:
    std::string sourceParam_, channelParam_;
    unsigned lastGen_ = ~0u;
};

class OscLog {
public:
    explicit OscLog(std::string directionParam) : directionParam_(std::move(directionParam)) {}

    bool drain(ModelHost& host, const std::string& organism, bool paused, EventLog& log) {
        auto* src = live::source<OscLogSource>(host, organism);
        if (src == nullptr || src->oscLogGeneration() == lastGen_) return false;
        lastGen_ = src->oscLogGeneration();

        OscLogSource::Logged events[256];
        const int count = src->consumeOscLog(events, 256);
        if (count == 0 || paused) return false;

        const int dir = (int) host.liveParamValue(organism, directionParam_);
        bool pushed = false;
        for (int i = 0; i < count; ++i) {
            const auto& e = events[i];
            if (dir == 1 && e.out) continue;
            if (dir == 2 && !e.out) continue;
            log.push(std::string(e.out ? "out  " : "in   ") + e.address + "  " + e.args, e.out);
            pushed = true;
        }
        return pushed;
    }

private:
    std::string directionParam_;
    unsigned lastGen_ = ~0u;
};

}
