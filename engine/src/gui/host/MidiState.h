// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>

#include "core/midi/MidiControl.h"
#include "core/midi/MidiControlFeed.h"
#include "hum/caps/Midi.h"
#include "core/midi/MidiRecord.h"
#include "core/midi/MidiSource.h"

namespace hum {

class PluginNode;

struct MidiState {
    static constexpr int kPorts = kMidiPorts;

    struct Target {
        PluginNode* hp;
        int mode;
        int channel;
    };
    struct RecordTarget {
        std::string node;
        int clip = -1;
        int quantize = 0;
        bool thru = false;
        bool grow = false;
        bool fromInput = false;
        std::vector<int> ports;
        bool inlet = false;
        int takeColour = kNoColour;

        bool hearsPort(int port) const {
            if (!fromInput) return !inlet;
            if (port < 0) return !ports.empty();
            for (int p : ports)
                if (p == kLiveMidiAllPorts || p == port) return true;
            return false;
        }

        bool hears(const TimedMidi& e, int graphIndex) const {
            if (e.node >= 0) return inlet && e.node == graphIndex;
            return hearsPort(e.port);
        }
    };

    std::vector<std::unique_ptr<juce::MidiInput>> inputs;
    std::vector<int> inputPorts;
    std::unique_ptr<juce::MidiOutput> outs[kPorts];
    MidiControlMap map;
    bool enabled = false;
    std::atomic<int> ccValue[kMidiPortRows][kMidiSourceCount];
    std::atomic<int> lastCc{-1};
    std::atomic<int> lastPort{kAnyMidiPort};
    std::atomic<int> lastChannel{kAnyMidiChannel};
    MidiControlInbox inbox;
    MidiControlFeed feed;
    std::vector<MidiControlEvent> drained;
    juce::CriticalSection targetsLock;
    std::vector<Target> targets;
    std::vector<RecordTarget> recordTargets;
    bool recordUndoPushed = false;
    std::vector<TimedMidi> capture;
    std::map<std::string, unsigned> keyboardHits;
    std::set<std::string> silenced;

    juce::MidiOutput* outForPort(int p) { return p >= 0 && p < kPorts ? outs[p].get() : nullptr; }
    bool anyOutOpen() const {
        for (const auto& o : outs)
            if (o) return true;
        return false;
    }
};

}
