// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/graph/AudioGraph.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstddef>
#include <queue>
#include <typeinfo>

#include "core/graph/AdoptSlot.h"
#include "core/tuning/MtsTuning.h"
#include "core/plugins/PluginNode.h"
#include "core/graph/RtWord.h"

#include "hum/Swing.h"
#include "hum/dsp/DspMath.h"

namespace hum {

int AudioGraph::addNode(OrganismPtr c) {
    Node n;
    n.c = std::move(c);
    nodes_.push_back(std::move(n));
    return (int) nodes_.size() - 1;
}

static void appendInbound(std::vector<std::vector<int>>& lists, int node, int index) {
    if (node < 0) return;
    if (node >= (int) lists.size()) lists.resize((size_t) node + 1);
    lists[(size_t) node].push_back(index);
}

void AudioGraph::connect(int srcNode, int srcOutlet, int dstNode, int dstInlet) {
    cords_.push_back({srcNode, srcOutlet, dstNode, dstInlet});
    appendInbound(inCords_, dstNode, (int) cords_.size() - 1);
}

void AudioGraph::connectMidi(int srcNode, int srcPort, int dstNode, int dstPort, int channel) {
    midiCords_.push_back({srcNode, srcPort, dstNode, dstPort, channel});
    appendInbound(inMidiCords_, dstNode, (int) midiCords_.size() - 1);
}

const std::vector<int>& AudioGraph::inboundCords(int node) const {
    static const std::vector<int> none;
    return node >= 0 && node < (int) inCords_.size() ? inCords_[(size_t) node] : none;
}

const std::vector<int>& AudioGraph::inboundMidiCords(int node) const {
    static const std::vector<int> none;
    return node >= 0 && node < (int) inMidiCords_.size() ? inMidiCords_[(size_t) node] : none;
}

int AudioGraph::indexOf(const std::string& name) const {
    for (int i = 0; i < (int) nodes_.size(); ++i)
        if (nodes_[i].c && nodes_[i].c->name() == name) return i;
    return -1;
}

Organism* AudioGraph::find(const std::string& name) {
    int i = indexOf(name);
    return i < 0 ? nullptr : nodes_[i].c.get();
}

void AudioGraph::computeOrder() {
    const int n = (int) nodes_.size();
    auto kahn = [&](bool withRoutes, std::vector<int>& out) {
        std::vector<std::vector<int>> succ((size_t) n);
        std::vector<int> indeg((size_t) n, 0);
        std::vector<std::vector<char>> seen((size_t) n, std::vector<char>((size_t) n, 0));
        auto addEdge = [&](int src, int dst) {
            if (src < 0 || dst < 0 || src >= n || dst >= n || src == dst) return;
            if (seen[(size_t) src][(size_t) dst]) return;
            seen[(size_t) src][(size_t) dst] = 1;
            succ[(size_t) src].push_back(dst);
            indeg[(size_t) dst]++;
        };
        for (const auto& c : cords_) addEdge(c.srcNode, c.dstNode);
        for (const auto& c : midiCords_) addEdge(c.srcNode, c.dstNode);
        if (withRoutes)
            for (const auto& r : modRoutes_) addEdge(r.srcNode, r.dstNode);
        std::queue<int> q;
        for (int i = 0; i < n; ++i) if (indeg[(size_t) i] == 0) q.push(i);
        out.clear();
        while (!q.empty()) {
            int u = q.front(); q.pop();
            out.push_back(u);
            for (int v : succ[(size_t) u]) if (--indeg[(size_t) v] == 0) q.push(v);
        }
        const bool complete = (int) out.size() == n;
        for (int i = 0; i < n; ++i)
            if (std::find(out.begin(), out.end(), i) == out.end()) out.push_back(i);
        return complete;
    };
    std::vector<int> cordsOnly;
    hasFeedback_ = !kahn(false, cordsOnly);
    if (modRoutes_.empty()) { order_ = std::move(cordsOnly); return; }
    kahn(true, order_);
}

void AudioGraph::prepare(double sampleRate, int maxBlock, double tempoBpm) {
    maxBlock_ = maxBlock;
    transport_.prepare(sampleRate, tempoBpm);
    transport_.reset();
    prepareNodes(sampleRate, maxBlock, nullptr);
}

void AudioGraph::prepare(double sampleRate, int maxBlock, double tempoBpm,
                         const AudioGraph& handover) {
    maxBlock_ = maxBlock;
    transport_.prepare(sampleRate, tempoBpm);
    transport_.reset();
    prepareNodes(sampleRate, maxBlock, &handover);
}

void AudioGraph::prepareNodes(double sampleRate, int maxBlock, const AudioGraph* handover) {
    std::vector<int> maxInOf(nodes_.size(), 0), maxOutOf(nodes_.size(), 0);
    for (const auto& c : cords_) {
        if (c.dstNode >= 0 && c.dstNode < (int) nodes_.size())
            maxInOf[(size_t) c.dstNode] = std::max(maxInOf[(size_t) c.dstNode], c.dstInlet + 1);
        if (c.srcNode >= 0 && c.srcNode < (int) nodes_.size())
            maxOutOf[(size_t) c.srcNode] = std::max(maxOutOf[(size_t) c.srcNode], c.srcOutlet + 1);
    }
    for (int i = 0; i < (int) nodes_.size(); ++i) {
        nodes_[i].c->configureChannels(maxInOf[(size_t) i], maxOutOf[(size_t) i]);
        nodes_[i].inChannels = nodes_[i].c->numAudioInputs();
        nodes_[i].outChannels = nodes_[i].c->numAudioOutputs();
        nodes_[i].midi = dynamic_cast<MidiNode*>(nodes_[i].c.get());
        nodes_[i].midiIns = nodes_[i].midi ? nodes_[i].midi->numMidiInputs() : 0;
        nodes_[i].midiOuts = nodes_[i].midi ? nodes_[i].midi->numMidiOutputs() : 0;
        if (handover == nullptr || adoptableFrom(i, *handover) < 0)
            nodes_[i].c->prepare(sampleRate, maxBlock);

        nodes_[i].outBuf.assign((size_t) nodes_[i].outChannels, std::vector<float>((size_t) maxBlock, 0.0f));
        nodes_[i].inBuf.assign((size_t) nodes_[i].inChannels, std::vector<float>((size_t) maxBlock, 0.0f));
        nodes_[i].inPtrs.resize((size_t) nodes_[i].inChannels);
        nodes_[i].outPtrs.resize((size_t) nodes_[i].outChannels);
        for (int c = 0; c < nodes_[i].inChannels; ++c) nodes_[i].inPtrs[(size_t) c] = nodes_[i].inBuf[(size_t) c].data();
        for (int c = 0; c < nodes_[i].outChannels; ++c) nodes_[i].outPtrs[(size_t) c] = nodes_[i].outBuf[(size_t) c].data();
        if (dynamic_cast<WantsNullInlets*>(nodes_[i].c.get()) != nullptr)
            for (int c = 0; c < nodes_[i].inChannels; ++c) {
                bool fed = false;
                for (int ci : inboundCords(i))
                    if (cords_[(size_t) ci].dstInlet == c) { fed = true; break; }
                if (!fed) nodes_[i].inPtrs[(size_t) c] = nullptr;
            }

        Node& nd = nodes_[i];
        nd.midi = dynamic_cast<MidiNode*>(nd.c.get());
        nd.latency = dynamic_cast<LatencyReporting*>(nd.c.get());
        nd.tuningProvider = dynamic_cast<TuningProvider*>(nd.c.get());
        nd.plugin = dynamic_cast<PluginNode*>(nd.c.get());
        nd.midiIns = nd.midi ? nd.midi->numMidiInputs() : 0;
        nd.midiOuts = nd.midi ? nd.midi->numMidiOutputs() : 0;
        nd.midiOut.assign((size_t) nd.midiOuts,
                          std::vector<MidiEvent>(MidiNode::kMaxMidiEventsPerBlock));
        nd.midiOutCount.assign((size_t) nd.midiOuts, 0);
    }
    midiScratch_.assign(MidiNode::kMaxMidiEventsPerBlock, MidiEvent{});
    midiScratch2_.assign(2 * MidiNode::kMaxMidiEventsPerBlock, MidiEvent{});
    trackEdges_.assign(MidiNode::kMaxMidiEventsPerBlock, noteschedule::Edge{});
    for (auto& nd : nodes_) nd.bendRetuner.reset();
    computeOrder();
    computeLatencyCompensation();
    resolveTunings();
    prepared_ = true;
}

void AudioGraph::processBlock(int numSamples) {
    if (!externalTempo_.load(std::memory_order_relaxed))
        for (int node : order_) {
            const double t = nodes_[node].c->masterTempo();
            if (t > 0.0) { transport_.setTempo(t); break; }
        }
    applyAutomation();
    if (transport_.meterMapped()) transport_.setBeatsPerBar(transport_.meter().quarterNotesPerBar());

    for (auto& nd : nodes_)
        if (nd.tuningProvider != nullptr) nd.tuningProvider->refreshTuning();

    const Tuning* defaultTuning =
        defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr;
    for (int node : order_) {
        Node& nd = nodes_[node];

        for (auto& ch : nd.inBuf) std::fill(ch.begin(), ch.begin() + numSamples, 0.0f);
        for (int cordIndex : inboundCords(node)) {
            const size_t ci = (size_t) cordIndex;
            const Cord& c = cords_[ci];
            if (c.dstInlet >= nd.inChannels) continue;
            Node& src = nodes_[c.srcNode];
            if (c.srcOutlet >= src.outChannels) continue;
            const float* s = src.outBuf[c.srcOutlet].data();
            float* d = nd.inBuf[c.dstInlet].data();
            CordDelay& cd = cordDelay_[ci];
            if (cd.buf.empty()) {
                for (int i = 0; i < numSamples; ++i) d[i] += s[i];
            } else {
                const int D = (int) cd.buf.size();
                int p = cd.pos;
                for (int i = 0; i < numSamples; ++i) {
                    const float out = cd.buf[(size_t) p];
                    cd.buf[(size_t) p] = s[i];
                    if (++p >= D) p = 0;
                    d[i] += out;
                }
                cd.pos = p;
            }
        }

        if (rtLoadWord(nd.bypass)) {
            passThrough(nd, node, numSamples);
        } else {
            if (nd.midi) routeMidiInto(nd, node, numSamples);

            transport_.setActiveTuning(nd.tuning != nullptr ? nd.tuning : defaultTuning);
            applyModRoutesInto(node, numSamples);
            nd.c->process(nd.inPtrs.data(), nd.inChannels,
                          nd.outPtrs.data(), nd.outChannels,
                          numSamples, transport_);

            int midiEmitted = 0;
            for (int p = 0; p < nd.midiOuts; ++p) {
                nd.midiOutCount[(size_t) p] = nd.midi->collectMidi(
                    p, nd.midiOut[(size_t) p].data(), MidiNode::kMaxMidiEventsPerBlock);
                midiEmitted += nd.midiOutCount[(size_t) p];
            }
            if (midiEmitted > 0)
                rtStoreWord(nd.actMidi, nd.actMidi + (float) midiEmitted);
        }

        {
            const auto& bufs = nd.outChannels > 0 ? nd.outBuf : nd.inBuf;
            const int chs = nd.outChannels > 0 ? nd.outChannels : nd.inChannels;
            float peak = 0.0f;
            for (int ch = 0; ch < chs; ++ch) {
                const float* s = bufs[(size_t) ch].data();
                for (int i = 0; i < numSamples; i += 8) peak = std::max(peak, std::abs(s[i]));
            }
            rtStoreWord(nd.actAudio, peak);
        }

        if (capturing_ && !nd.capBuf.empty()) {
            const int n = std::min((int) nd.capBuf.size() - nd.capPos, numSamples);
            if (n > 0) {
                const auto& bufs = nd.outChannels > 0 ? nd.outBuf : nd.inBuf;
                const int chs = nd.outChannels > 0 ? nd.outChannels : nd.inChannels;
                float* w = nd.capBuf.data() + nd.capPos;
                const float norm = chs > 0 ? 1.0f / (float) chs : 0.0f;
                for (int i = 0; i < n; ++i) w[i] = 0.0f;
                for (int ch = 0; ch < chs; ++ch) {
                    const float* s = bufs[(size_t) ch].data();
                    for (int i = 0; i < n; ++i) w[i] += s[i] * norm;
                }
                nd.capPos += n;
            }
        }
    }
    transport_.setActiveTuning(defaultTuning);
    transport_.advance(numSamples);
}

void AudioGraph::armCapture(int samples) {
    for (auto& nd : nodes_) {
        nd.capBuf.assign((size_t) std::max(0, samples), 0.0f);
        nd.capPos = 0;
    }
    capturing_ = true;
}

void AudioGraph::disarmCapture() { capturing_ = false; }

int AudioGraph::captureFill(int node) const {
    return node >= 0 && node < (int) nodes_.size() ? nodes_[(size_t) node].capPos : 0;
}

const float* AudioGraph::captureData(int node) const {
    return node >= 0 && node < (int) nodes_.size() ? nodes_[(size_t) node].capBuf.data()
                                                   : nullptr;
}

float AudioGraph::nodeAudioActivity(int node) const {
    return node >= 0 && node < (int) nodes_.size()
               ? rtLoadWord(nodes_[(size_t) node].actAudio) : 0.0f;
}

float AudioGraph::nodeMidiActivity(int node) const {
    return node >= 0 && node < (int) nodes_.size()
               ? rtLoadWord(nodes_[(size_t) node].actMidi) : 0.0f;
}

void AudioGraph::setNodeTrack(int node, std::vector<noteschedule::Voice> voices) {
    if (node < 0 || node >= (int) nodes_.size()) return;
    auto& nd = nodes_[(std::size_t) node];
    nd.trackSwapped = true;
    nd.track = std::move(voices);
}

void AudioGraph::setNodeBypass(int node, bool on) {
    if (node < 0 || node >= (int) nodes_.size()) return;
    if (on) rtStoreWord(nodes_[(size_t) node].bypassFlush, true);
    rtStoreWord(nodes_[(size_t) node].bypass, on);
}

bool AudioGraph::nodeBypass(int node) const {
    if (node < 0 || node >= (int) nodes_.size()) return false;
    return rtLoadWord(nodes_[(size_t) node].bypass);
}

void AudioGraph::passThrough(Node& nd, int node, int numSamples) {
    if (rtLoadWord(nd.bypassFlush)) {
        for (auto& ch : nd.bypassRing) std::fill(ch.begin(), ch.end(), 0.0f);
        nd.bypassPos = 0;
        rtStoreWord(nd.bypassFlush, false);
    }

    const int through = std::min(nd.inChannels, nd.outChannels);
    const int lat = nd.bypassRing.empty() ? 0 : (int) nd.bypassRing[0].size();
    for (int ch = 0; ch < through; ++ch) {
        const float* s = nd.inBuf[(size_t) ch].data();
        float* d = nd.outBuf[(size_t) ch].data();
        if (lat == 0) { std::copy(s, s + numSamples, d); continue; }
        float* ring = nd.bypassRing[(size_t) ch].data();
        int p = nd.bypassPos;
        for (int i = 0; i < numSamples; ++i) {
            d[i] = ring[p];
            ring[p] = s[i];
            if (++p >= lat) p = 0;
        }
    }
    if (lat > 0) nd.bypassPos = (nd.bypassPos + numSamples) % lat;
    for (int ch = through; ch < nd.outChannels; ++ch)
        std::fill(nd.outBuf[(size_t) ch].begin(),
                  nd.outBuf[(size_t) ch].begin() + numSamples, 0.0f);

    const int ports = std::min(nd.midiIns, nd.midiOuts);
    int forwarded = 0;
    for (int p = 0; p < ports; ++p) {
        const int count = gatherMidiFor(node, p);
        std::copy(midiScratch_.begin(), midiScratch_.begin() + count,
                  nd.midiOut[(size_t) p].begin());
        nd.midiOutCount[(size_t) p] = count;
        forwarded += count;
    }
    for (int p = ports; p < nd.midiOuts; ++p) nd.midiOutCount[(size_t) p] = 0;
    if (forwarded > 0) rtStoreWord(nd.actMidi, nd.actMidi + (float) forwarded);
}

}
