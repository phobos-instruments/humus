// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/graph/AudioGraph.h"

#include <algorithm>
#include <cstddef>

#include "core/graph/RtWord.h"

namespace hum {

void AudioGraph::setNodeBypass(int node, bool on) {
    if (node < 0 || node >= (int) nodes_.size()) return;
    rtStoreWord(nodes_[(size_t) node].bypass, on);
}

bool AudioGraph::nodeBypass(int node) const {
    if (node < 0 || node >= (int) nodes_.size()) return false;
    return rtLoadWord(nodes_[(size_t) node].bypass);
}

void AudioGraph::runBypassable(Node& nd, int node, int numSamples) {
    const bool bypassed = rtLoadWord(nd.bypass);
    const float target = bypassed ? 1.0f : 0.0f;
    if (!nd.bypassMixKnown || nd.outChannels == 0) {
        nd.bypassMix = target;
        nd.bypassMixKnown = true;
    }
    if (nd.bypassMix != target) {
        crossfadeBypass(nd, node, numSamples, target);
    } else if (bypassed) {
        passAudio(nd, nd.outPtrs.data(), numSamples);
        passMidi(nd, node);
        dropWhileBypassed(nd);
    } else {
        processNode(nd, node, numSamples);
        feedBypassRing(nd, numSamples);
    }
}

void AudioGraph::processNode(Node& nd, int node, int numSamples) {
    if (nd.midi) routeMidiInto(nd, node, numSamples);
    applyModRoutesInto(node, numSamples);
    applyNamedTaps(node);
    nd.c->process(nd.inPtrs.data(), nd.inChannels, nd.outPtrs.data(), nd.outChannels, numSamples,
                  transport_);

    int midiEmitted = 0;
    for (int p = 0; p < nd.midiOuts; ++p) {
        nd.midiOutCount[(size_t) p] =
            nd.midi->collectMidi(p, nd.midiOut[(size_t) p].data(), MidiNode::kMaxMidiEventsPerBlock);
        midiEmitted += nd.midiOutCount[(size_t) p];
    }
    if (midiEmitted > 0) rtStoreWord(nd.actMidi, nd.actMidi + (float) midiEmitted);
}

void AudioGraph::crossfadeBypass(Node& nd, int node, int numSamples, float target) {
    processNode(nd, node, numSamples);
    passAudio(nd, dryPtrs_.data(), numSamples);
    if (target > 0.5f) passMidi(nd, node);

    const float step = target > nd.bypassMix ? bypassStep_ : -bypassStep_;
    float mix = nd.bypassMix;
    for (int i = 0; i < numSamples; ++i) {
        mix = step > 0.0f ? std::min(target, mix + step) : std::max(target, mix + step);
        for (int ch = 0; ch < nd.outChannels; ++ch) {
            float& wet = nd.outBuf[(size_t) ch][(size_t) i];
            wet += (dryScratch_[(size_t) ch][(size_t) i] - wet) * mix;
        }
    }
    nd.bypassMix = mix;
}

void AudioGraph::passAudio(Node& nd, float* const* dst, int numSamples) {
    const int through = std::min(nd.inChannels, nd.outChannels);
    const int lat = nd.bypassRing.empty() ? 0 : (int) nd.bypassRing[0].size();
    for (int ch = 0; ch < through; ++ch) {
        const float* s = nd.inBuf[(size_t) ch].data();
        float* d = dst[ch];
        if (lat == 0) {
            std::copy(s, s + numSamples, d);
            continue;
        }
        float* ring = nd.bypassRing[(size_t) ch].data();
        int p = nd.bypassPos;
        for (int i = 0; i < numSamples; ++i) {
            d[i] = ring[p];
            ring[p] = s[i];
            if (++p >= lat) p = 0;
        }
    }
    if (lat > 0) nd.bypassPos = (nd.bypassPos + numSamples) % lat;
    for (int ch = through; ch < nd.outChannels; ++ch) std::fill(dst[ch], dst[ch] + numSamples, 0.0f);
}

void AudioGraph::feedBypassRing(Node& nd, int numSamples) {
    const int lat = nd.bypassRing.empty() ? 0 : (int) nd.bypassRing[0].size();
    if (lat == 0) return;
    const int through = std::min(nd.inChannels, (int) nd.bypassRing.size());
    for (int ch = 0; ch < through; ++ch) {
        const float* s = nd.inBuf[(size_t) ch].data();
        float* ring = nd.bypassRing[(size_t) ch].data();
        int p = nd.bypassPos;
        for (int i = 0; i < numSamples; ++i) {
            ring[p] = s[i];
            if (++p >= lat) p = 0;
        }
    }
    nd.bypassPos = (nd.bypassPos + numSamples) % lat;
}

void AudioGraph::passMidi(Node& nd, int node) {
    const int ports = std::min(nd.midiIns, nd.midiOuts);
    int forwarded = 0;
    for (int p = 0; p < ports; ++p) {
        const int count = gatherMidiFor(node, p);
        std::copy(midiScratch_.begin(), midiScratch_.begin() + count, nd.midiOut[(size_t) p].begin());
        nd.midiOutCount[(size_t) p] = count;
        forwarded += count;
    }
    for (int p = ports; p < nd.midiOuts; ++p) nd.midiOutCount[(size_t) p] = 0;
    if (forwarded > 0) rtStoreWord(nd.actMidi, nd.actMidi + (float) forwarded);
}

}
