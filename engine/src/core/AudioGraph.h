#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/Automation.h"
#include "core/BendRetuner.h"
#include "hum/Capabilities.h"
#include "hum/NoteSchedule.h"
#include "hum/Organism.h"
#include "hum/Transport.h"

namespace hum {

class PluginNode;

class AudioGraph {
public:
    struct Cord { int srcNode; int srcOutlet; int dstNode; int dstInlet; };
    struct MidiCord { int srcNode; int srcPort; int dstNode; int dstPort; int channel; };

    int addNode(OrganismPtr c);
    void connect(int srcNode, int srcOutlet, int dstNode, int dstInlet);
    void connectMidi(int srcNode, int srcPort, int dstNode, int dstPort, int channel = 0);

    int nodeCount() const { return (int) nodes_.size(); }
    Organism* organism(int node) { return nodes_[node].c.get(); }
    Organism* find(const std::string& name);
    int indexOf(const std::string& name) const;

    void prepare(double sampleRate, int maxBlock, double tempoBpm);
    void prepare(double sampleRate, int maxBlock, double tempoBpm, const AudioGraph& handover);
    int adoptableFrom(int node, const AudioGraph& old) const;
    int preparedBlock() const { return maxBlock_; }
    int maxTrackHeld() const { return maxTrackHeld_; }
    void resetMaxTrackHeld() { maxTrackHeld_ = 0; }
    void processBlock(int numSamples);

    void adoptMatchingNodes(AudioGraph& old);

    void setAutomation(std::vector<AutoLane> lanes) { autoLanes_ = std::move(lanes); }

    Transport& transport() { return transport_; }

    void setExternalTempoMaster(bool on) { externalTempo_.store(on, std::memory_order_relaxed); }

    int outputChannels(int node) const {
        return node >= 0 && node < (int) nodes_.size() ? nodes_[(size_t) node].outChannels : 0;
    }
    const float* outputData(int node, int channel) const {
        if (channel < 0 || channel >= outputChannels(node)) return nullptr;
        return nodes_[(size_t) node].outBuf[(size_t) channel].data();
    }

    const std::vector<int>& order() const { return order_; }
    bool hasFeedback() const { return hasFeedback_; }

    void setNodeBypass(int node, bool on);
    bool nodeBypass(int node) const;

    void setNodeTrack(int node, std::vector<noteschedule::Voice> voices);
    void setNodeTrackMuted(int node, bool muted);
    void pushLiveMidi(int node, const MidiEvent& e);
    bool acceptsLiveMidi(int node) const;
    bool hasNodeTrack(int node) const {
        return node >= 0 && node < (int) nodes_.size() && !nodes_[(size_t) node].track.empty();
    }

    float nodeAudioActivity(int node) const;
    float nodeMidiActivity(int node) const;
    int nodeMidiOut(int node, int port, const MidiEvent*& out) const {
        if (node < 0 || node >= (int) nodes_.size()) return 0;
        const Node& nd = nodes_[(size_t) node];
        if (port < 0 || port >= nd.midiOuts) return 0;
        out = nd.midiOut[(size_t) port].data();
        return nd.midiOutCount[(size_t) port];
    }

    void armCapture(int samples);
    void disarmCapture();
    bool capturing() const { return capturing_; }
    int captureFill(int node) const;
    const float* captureData(int node) const;

private:
    struct Node {
        OrganismPtr c;
        int inChannels = 0;
        int outChannels = 0;
        int latencySamples = 0;
        std::vector<std::vector<float>> outBuf;
        std::vector<std::vector<float>> inBuf;

        std::vector<const float*> inPtrs;
        std::vector<float*> outPtrs;
        float actAudio = 0.0f;
        float actMidi = 0.0f;
        bool bypass = false;
        bool bypassFlush = false;
        std::vector<std::vector<float>> bypassRing;
        int bypassPos = 0;
        std::vector<float> capBuf;
        int capPos = 0;
        std::vector<noteschedule::Voice> track;
        std::vector<bool> trackHeld = std::vector<bool>(128, false);
        bool trackWasPlaying = false;
        bool trackSwapped = false;
        std::uint32_t trackSeenSeek = 0;
        bool trackMuted = false;
        struct LiveQueue {
            std::mutex m;
            std::array<MidiEvent, 64> buf{};
            int count = 0;
        };
        std::unique_ptr<LiveQueue> live;
        MidiNode* midi = nullptr;
        TuningProvider* tuningProvider = nullptr;
        PluginNode* plugin = nullptr;
        Tuning sentTuning;
        BendRetuner bendRetuner;
        bool bendVerdict = true;
        bool verdictKnown = false;
        const Tuning* tuning = nullptr;
        int midiIns = 0, midiOuts = 0;
        std::vector<std::vector<MidiEvent>> midiOut;
        std::vector<int> midiOutCount;
    };

    void prepareNodes(double sampleRate, int maxBlock, const AudioGraph* handover);
    void computeOrder();
    void computeLatencyCompensation();
    void resolveTunings();

public:
    void pumpPluginTunings();

    std::vector<std::string> pluginsNeedingVerdict() const;
    void setBendVerdict(const std::string& classRaw, bool needsBend);

private:
    void routeMidiInto(Node& nd, int node, int numSamples);
    int appendTrackMidi(Node& nd, int count, int numSamples);
    int appendLiveMidi(Node& nd, int count);
    std::vector<noteschedule::Edge> trackEdges_;
    void passThrough(Node& nd, int node, int numSamples);
    int gatherMidiFor(int node, int port);

    struct CordDelay { std::vector<float> buf; int pos = 0; };

    void applyAutomation();

    std::vector<Node> nodes_;
    std::vector<Cord> cords_;
    std::vector<CordDelay> cordDelay_;
    std::vector<MidiCord> midiCords_;
    TuningProvider* defaultTuning_ = nullptr;
    std::vector<MidiEvent> midiScratch_;
    std::vector<MidiEvent> midiScratch2_;
    std::vector<int> order_;
    std::vector<AutoLane> autoLanes_;
    Transport transport_;
    std::atomic<bool> externalTempo_{false};
    bool capturing_ = false;
    double lastAutoBeat_ = 0.0;
    int maxBlock_ = 0;
    int maxTrackHeld_ = 0;
    bool hasFeedback_ = false;
    bool prepared_ = false;
};

}
