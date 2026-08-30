#pragma once
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "core/AudioGraph.h"
#include "core/PluginNode.h"
#include "hum/Capabilities.h"
#include "io/PatchDocument.h"

namespace hum::graphmidi {

inline bool toEvent(const juce::MidiMessage& m, MidiEvent& out, int sampleOffset = 0) {
    const int n = m.getRawDataSize();
    if (n < 1 || n > 3) return false;
    const auto* raw = m.getRawData();
    for (int i = 0; i < n; ++i) out.data[i] = raw[i];
    out.size = n;
    out.sampleOffset = sampleOffset;
    return true;
}

inline juce::MidiMessage toMessage(const MidiEvent& e) {
    return e.size == 1 ? juce::MidiMessage(e.data[0])
         : e.size == 2 ? juce::MidiMessage(e.data[0], e.data[1])
                       : juce::MidiMessage(e.data[0], e.data[1], e.data[2]);
}

struct Hosted {
    PluginNode* node = nullptr;
    std::string name;
    int receiveMode = OrganismModel::kMidiCordsOnly;
    int channel = 1;
};

struct Ports {
    std::vector<LiveMidiIn*> ins;
    std::vector<PendingMidiOut*> drains;
    std::vector<Hosted> hosted;
};

inline Ports findPorts(AudioGraph& g) {
    Ports p;
    for (int i = 0; i < g.nodeCount(); ++i) {
        auto* node = g.organism(i);
        if (auto* in = dynamic_cast<LiveMidiIn*>(node)) p.ins.push_back(in);
        if (auto* out = dynamic_cast<PendingMidiOut*>(node)) p.drains.push_back(out);
    }
    return p;
}

inline bool wantsLiveMidi(const Hosted& h, const juce::MidiMessage& m) {
    if (h.receiveMode == OrganismModel::kMidiCordsOnly) return false;
    if (h.receiveMode == OrganismModel::kMidiChannel)
        return m.getChannel() == 0 || m.getChannel() == h.channel;
    return true;
}

inline void deliver(const juce::MidiBuffer& from, const Ports& ports) {
    for (const auto meta : from) {
        const auto m = meta.getMessage();
        for (const auto& h : ports.hosted)
            if (wantsLiveMidi(h, m)) h.node->queueMidiMessage(m);
        MidiEvent e;
        if (!toEvent(m, e, meta.samplePosition)) continue;
        for (auto* in : ports.ins) in->pushLiveMidi(e);
    }
}

inline void collect(const Ports& ports, juce::MidiBuffer& into) {
    MidiEvent buf[MidiNode::kMaxMidiEventsPerBlock];
    for (auto* drain : ports.drains) {
        const int n = drain->consumeOutput(buf, MidiNode::kMaxMidiEventsPerBlock);
        for (int i = 0; i < n; ++i) {
            if (buf[i].size < 1) continue;
            into.addEvent(toMessage(buf[i]), juce::jmax(0, buf[i].sampleOffset));
        }
    }
}

}
