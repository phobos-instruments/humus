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

void AudioGraph::computeLatencyCompensation() {
    const int n = (int) nodes_.size();
    cordDelay_.assign(cords_.size(), CordDelay{});
    for (int i = 0; i < n; ++i) {
        auto* lr = nodes_[i].latency;
        nodes_[i].latencySamples = lr ? std::max(0, lr->latencySamples()) : 0;
        const size_t ring = (size_t) std::min<long long>(nodes_[i].latencySamples, 1 << 20);
        nodes_[i].bypassRing.assign(ring > 0 ? (size_t) nodes_[i].outChannels : 0,
                                    std::vector<float>(ring, 0.0f));
        nodes_[i].bypassPos = 0;
    }
    std::vector<int> orderPos(n, 0);
    for (int k = 0; k < n; ++k) orderPos[order_[k]] = k;
    std::vector<long long> inLat(n, 0), outLat(n, 0);
    for (int node : order_) {
        long long mx = 0;
        for (int ci : inboundCords(node)) {
            const auto& c = cords_[(size_t) ci];
            if (orderPos[c.srcNode] >= orderPos[node]) continue;
            mx = std::max(mx, outLat[c.srcNode]);
        }
        inLat[node] = mx;
        outLat[node] = mx + nodes_[node].latencySamples;
    }
    const long long cap = 1 << 20;
    for (size_t ci = 0; ci < cords_.size(); ++ci) {
        const auto& c = cords_[ci];
        if (orderPos[c.srcNode] >= orderPos[c.dstNode]) continue;
        long long d = inLat[c.dstNode] - outLat[c.srcNode];
        d = std::max<long long>(0, std::min(cap, d));
        cordDelay_[ci].buf.assign((size_t) d, 0.0f);
        cordDelay_[ci].pos = 0;
    }
}

int AudioGraph::adoptableFrom(int node, const AudioGraph& old) const {
    const Node& n = nodes_[(size_t) node];
    if (!n.c) return -1;
    const int oi = old.indexOf(n.c->name());
    if (oi < 0) return -1;
    const Node& on = old.nodes_[(size_t) oi];
    if (!on.c) return -1;
    const bool slot = dynamic_cast<const AdoptSlot*>(n.c.get()) != nullptr;
    if (slot && n.c->matchToken().empty()) return -1;
    const Organism& oldC = *on.c;
    const Organism& newC = *n.c;
    if (!slot && typeid(oldC) != typeid(newC)) return -1;
    if (on.c->matchToken() != n.c->matchToken()) return -1;
    if (on.c->numAudioInputs() != n.inChannels) return -1;
    if (on.c->numAudioOutputs() != n.outChannels) return -1;
    auto* om = dynamic_cast<const MidiNode*>(on.c.get());
    if ((om ? om->numMidiInputs() : 0) != n.midiIns) return -1;
    if ((om ? om->numMidiOutputs() : 0) != n.midiOuts) return -1;
    return oi;
}

void AudioGraph::adoptMatchingNodes(AudioGraph& old) {
    for (int i = 0; i < (int) nodes_.size(); ++i) {
        auto& n = nodes_[(size_t) i];
        const int oi = adoptableFrom(i, old);
        if (oi < 0) continue;
        Node& on = old.nodes_[(size_t) oi];
        auto* om = dynamic_cast<MidiNode*>(on.c.get());
        std::swap(n.c, on.c);
        n.trackHeld = on.trackHeld;
        n.trackWasPlaying = on.trackWasPlaying;
        n.trackSwapped = true;
        n.trackSeenSeek = on.trackSeenSeek;
        n.midi = om;
        n.tuningProvider = dynamic_cast<TuningProvider*>(n.c.get());
        n.latency = dynamic_cast<LatencyReporting*>(n.c.get());
        n.plugin = dynamic_cast<PluginNode*>(n.c.get());
    }
    resolveTunings();
}

bool AudioGraph::takeLatencyChanges() {
    bool any = false;
    for (auto& nd : nodes_)
        if (nd.latency != nullptr && nd.latency->takeLatencyChange()) any = true;
    return any;
}

}
