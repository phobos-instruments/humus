// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <cstdint>

#include "hum/Extensions.h"

namespace hum {

struct MidiEvent {
    int sampleOffset = 0;
    unsigned char data[3] = {0, 0, 0};
    int size = 0;
    const MidiEventExt* ext = nullptr;
};

class MidiNode {
public:
    static constexpr int kMaxMidiEventsPerBlock = 512;
    virtual ~MidiNode() = default;
    virtual int numMidiInputs() const = 0;
    virtual int numMidiOutputs() const = 0;
    virtual void deliverMidi(int port, const MidiEvent* events, int count) = 0;
    virtual int collectMidi(int port, MidiEvent* out, int capacity) = 0;
};

class LiveMidiIn {
public:
    virtual ~LiveMidiIn() = default;
    virtual void pushLiveMidi(const MidiEvent& e) = 0;
    virtual int liveMidiPort() const { return 0; }
    virtual bool monitorsAllPorts() const { return false; }
    virtual void pushLiveMidi(const MidiEvent& e, int) { pushLiveMidi(e); }
};

class PendingMidiOut {
public:
    virtual ~PendingMidiOut() = default;
    virtual int consumeOutput(MidiEvent* out, int capacity) = 0;
    virtual int pendingMidiPort() const { return 0; }
};

class MidiLogSource {
public:
    struct Logged {
        MidiEvent event;
        unsigned char origin = 0;
        signed char port = -1;
    };
    virtual ~MidiLogSource() = default;
    virtual int consumeLog(Logged* dest, int maxEvents) = 0;
    virtual unsigned logGeneration() const = 0;
};

class StepStrip {
public:
    virtual ~StepStrip() = default;
    virtual int stripSteps() const = 0;
    virtual std::int64_t stripStepAt(double beats) const = 0;
    virtual bool stripStepRests(std::int64_t step) const = 0;
};

}
