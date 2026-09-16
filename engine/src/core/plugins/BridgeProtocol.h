// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

#include <juce_core/juce_core.h>

#include "hum/caps/Midi.h"

namespace hum {

constexpr uint32_t kBridgeMagic = 0x48554D42u;
constexpr uint32_t kBridgeVersion = 3;
constexpr int kBridgeMaxMidi = 512;
constexpr int kBridgeMaxParams = 256;
constexpr int kBridgeMaxBlock = 4096;

struct BridgeSignalStorage { alignas(8) unsigned char bytes[128]; };

struct BridgeParamChange { uint32_t index; float value; };

struct BridgeSlot {
    uint32_t numSamples = 0;
    double bpm = 120.0, ppq = 0.0;
    uint32_t playing = 0;
    uint32_t numMidiIn = 0, numMidiOut = 0, numParamChanges = 0;
    uint32_t latencySamples = 0;
    MidiEvent midiIn[kBridgeMaxMidi];
    MidiEvent midiOut[kBridgeMaxMidi];
    BridgeParamChange paramChanges[kBridgeMaxParams];
};

struct BridgeShmHeader {
    uint32_t magic = 0, version = 0;
    uint32_t numIns = 0, numOuts = 0;
    BridgeSignalStorage reqSignal, ackSignal;
    std::atomic<uint32_t> childReady{0};
    std::atomic<uint32_t> reqSeq{0};
    std::atomic<uint32_t> ackSeq{0};
    std::atomic<float> paramValues[kBridgeMaxParams];
    BridgeSlot slots[2];
};

inline size_t bridgeShmSize(int ins, int outs) {
    return sizeof(BridgeShmHeader)
         + 2ull * (size_t) (ins + outs) * kBridgeMaxBlock * sizeof(float);
}
inline float* bridgeSlotAudio(BridgeShmHeader* h, int slot, int plane) {
    auto* base = reinterpret_cast<float*>(reinterpret_cast<char*>(h) + sizeof(BridgeShmHeader));
    const int planes = (int) (h->numIns + h->numOuts);
    return base + ((size_t) slot * planes + plane) * kBridgeMaxBlock;
}

enum class BridgeMsg : uint32_t {
    Hello = 1,
    Prepare,
    MappedOk,
    GetState,
    State,
    SetState,
    Quit,
    CreateEditor,
    EditorCreated,
    DestroyEditor,
    SetFloating,
    SizeChanged,
    FloatClosed,
};

struct BridgeHello {
    int pid = 0, ins = 0, outs = 0, latency = 0, paramCount = 0;
    bool acceptsMidi = false, producesMidi = false;
};

inline juce::MemoryBlock bridgeMsg(BridgeMsg id,
                                   std::function<void(juce::MemoryOutputStream&)> body = {}) {
    juce::MemoryOutputStream os;
    os.writeInt((int) id);
    if (body) body(os);
    return os.getMemoryBlock();
}

}
