// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>

#include "core/midi/MidiControl.h"
#include "core/midi/MidiRecord.h"
#include "core/midi/MidiSource.h"

namespace hum {

class PluginNode;

struct MidiState {
    static constexpr int kPorts = 8;

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
    };

    std::vector<std::unique_ptr<juce::MidiInput>> inputs;
    std::vector<int> inputPorts;
    std::unique_ptr<juce::MidiOutput> outs[kPorts];
    MidiControlMap map;
    bool enabled = false;
    std::atomic<int> ccValue[kMidiSourceCount];
    std::atomic<int> lastCc{-1};
    int ccLastApplied[kMidiSourceCount];
    juce::CriticalSection targetsLock;
    std::vector<Target> targets;
    std::vector<RecordTarget> recordTargets;
    bool recordUndoPushed = false;
    std::vector<TimedMidi> capture;

    juce::MidiOutput* outForPort(int p) { return p >= 0 && p < kPorts ? outs[p].get() : nullptr; }
    bool anyOutOpen() const {
        for (const auto& o : outs)
            if (o) return true;
        return false;
    }
};

}
