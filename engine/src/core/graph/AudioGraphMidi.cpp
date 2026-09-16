// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/PatternMatrix.h"
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

static bool cordPasses(const AudioGraph::MidiCord& mc, const MidiEvent& e) {
    if (mc.channel <= 0) return true;
    const unsigned char status = e.data[0];
    if (status < 0x80 || status >= 0xF0) return true;
    return (status & 0x0F) == mc.channel - 1;
}

static bool trackSounding(const std::vector<noteschedule::Voice>& voices, int pitch, double tick) {
    for (const auto& v : voices) {
        if (tick < v.gateFrom || tick >= v.gateTo) continue;
        for (const auto& n : v.notes) {
            if (n.pitch != pitch) continue;
            double rel = tick - v.start;
            if (v.loop && v.len > 0) rel -= std::floor(rel / v.len) * v.len;
            if (rel >= n.tick && rel < n.tick + n.lengthTicks) return true;
        }
    }
    return false;
}

int AudioGraph::gatherMidiFor(int node, int port) {
    int count = 0;
    for (int mi : inboundMidiCords(node)) {
        const auto& mc = midiCords_[(size_t) mi];
        if (mc.dstPort != port) continue;
        const Node& src = nodes_[(size_t) mc.srcNode];
        if (mc.srcPort >= src.midiOuts) continue;
        const auto& ev = src.midiOut[(size_t) mc.srcPort];
        const int n = src.midiOutCount[(size_t) mc.srcPort];
        for (int i = 0; i < n && count < MidiNode::kMaxMidiEventsPerBlock; ++i)
            if (cordPasses(mc, ev[(size_t) i])) midiScratch_[(size_t) count++] = ev[(size_t) i];
    }
    if (count > 1)
        std::sort(midiScratch_.begin(), midiScratch_.begin() + count,
                  [](const MidiEvent& a, const MidiEvent& b) {
                      return a.sampleOffset < b.sampleOffset;
                  });
    return count;
}

int AudioGraph::appendTrackMidi(Node& nd, int count, int numSamples) {
    if (nd.track.empty()) return count;
    const bool playing = transport_.playing() && !nd.trackMuted;
    const bool jumped = nd.trackSeenSeek != transport_.seekStamp();
    nd.trackSeenSeek = transport_.seekStamp();
    if ((!playing && nd.trackWasPlaying) || nd.trackSwapped || jumped) {
        const bool releaseAll = !playing && nd.trackWasPlaying;
        const double tick = transport_.beats() * Pattern::kTicksPerBeat;
        for (int pitch = 0; pitch < 128; ++pitch) {
            if (!nd.trackHeld[(std::size_t) pitch]) continue;
            if (!releaseAll && trackSounding(nd.track, pitch, tick)) continue;
            nd.trackHeld[(std::size_t) pitch] = false;
            if (count >= MidiNode::kMaxMidiEventsPerBlock) break;
            MidiEvent& e = midiScratch_[(std::size_t) count++];
            e.sampleOffset = 0;
            e.data[0] = 0x80; e.data[1] = (unsigned char) pitch; e.data[2] = 0;
            e.size = 3;
        }
    }
    if (!playing && nd.trackWasPlaying && nd.trackBent && count < MidiNode::kMaxMidiEventsPerBlock) {
        nd.trackBent = false;
        MidiEvent& e = midiScratch_[(std::size_t) count++];
        e.sampleOffset = 0;
        fillControlEvent(e, 0, kBendController, kBendCentre);
    }
    nd.trackSwapped = false;
    nd.trackWasPlaying = playing;
    if (!playing) return count;

    const bool lp = transport_.loopEnabled();
    const int n = noteschedule::collect(nd.track, transport_.beats(), numSamples,
                                        transport_.samplesPerBeat(), 1.0,
                                        trackEdges_.data(), (int) trackEdges_.size(),
                                        lp ? transport_.loopStartBeat() : 0.0,
                                        lp ? transport_.loopEndBeat() : 0.0,
                                        transport_.groove());
    for (int i = 0; i < n && count < MidiNode::kMaxMidiEventsPerBlock; ++i) {
        const auto& ed = trackEdges_[(std::size_t) i];
        MidiEvent& e = midiScratch_[(std::size_t) count++];
        e.sampleOffset = ed.offset;
        e.size = 3;
        if (ed.cc >= 0) {
            fillControlEvent(e, 0, ed.cc, ed.vel);
            if (ed.cc == kBendController) nd.trackBent = ed.vel != kBendCentre;
            continue;
        }
        const int pitch = std::clamp(ed.pitch, 0, kMidiMax);
        if (!ed.on && !nd.trackHeld[(std::size_t) pitch]) { --count; continue; }
        nd.trackHeld[(std::size_t) pitch] = ed.on;
        if (ed.on) {
            int live = 0;
            for (bool h : nd.trackHeld) live += h ? 1 : 0;
            maxTrackHeld_ = std::max(maxTrackHeld_, live);
        }
        e.data[0] = (unsigned char) (ed.on ? 0x90 : 0x80);
        e.data[1] = (unsigned char) pitch;
        e.data[2] = (unsigned char) (ed.on ? std::clamp(ed.vel, 1, kMidiMax) : 0);
    }
    if (count > 1)
        std::stable_sort(midiScratch_.begin(), midiScratch_.begin() + count,
                         [](const MidiEvent& a, const MidiEvent& b) {
                             return a.sampleOffset < b.sampleOffset;
                         });
    return count;
}

void AudioGraph::pushLiveMidi(int node, const MidiEvent& e) {
    if (node < 0 || node >= (int) nodes_.size()) return;
    Node& nd = nodes_[(std::size_t) node];
    if (nd.midi == nullptr || nd.midiIns <= 0) return;
    if (!nd.live) nd.live = std::make_unique<Node::LiveQueue>();
    std::lock_guard<std::mutex> g(nd.live->m);
    if (nd.live->count < (int) nd.live->buf.size())
        nd.live->buf[(std::size_t) nd.live->count++] = e;
}

bool AudioGraph::acceptsLiveMidi(int node) const {
    if (node < 0 || node >= (int) nodes_.size()) return false;
    const Node& nd = nodes_[(std::size_t) node];
    return nd.midi != nullptr && nd.midiIns > 0;
}

int AudioGraph::appendLiveMidi(Node& nd, int count) {
    if (!nd.live) return count;
    std::unique_lock<std::mutex> g(nd.live->m, std::try_to_lock);
    if (!g.owns_lock() || nd.live->count == 0) return count;
    for (int i = 0; i < nd.live->count && count < MidiNode::kMaxMidiEventsPerBlock; ++i)
        midiScratch_[(std::size_t) count++] = nd.live->buf[(std::size_t) i];
    nd.live->count = 0;
    return count;
}

void AudioGraph::setNodeTrackMuted(int node, bool muted) {
    if (node < 0 || node >= (int) nodes_.size()) return;
    nodes_[(std::size_t) node].trackMuted = muted;
}

void AudioGraph::routeMidiInto(Node& nd, int node, int numSamples) {
    for (int p = 0; p < nd.midiIns; ++p) {
        int count = gatherMidiFor(node, p);
        if (p == 0) count = appendLiveMidi(nd, appendTrackMidi(nd, count, numSamples));
        if (nd.plugin != nullptr && count > 0) {
            const Tuning* act = nd.tuning != nullptr ? nd.tuning
                              : defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr;
            const int mode = act != nullptr ? act->pluginDelivery() : Tuning::kDeliverMts;
            const bool useBend = mode == Tuning::kDeliverBend
                              || (mode == Tuning::kDeliverAuto && nd.bendVerdict);
            if (act != nullptr && useBend && !act->isStandard()) {
                const int n2 = nd.bendRetuner.rewrite(midiScratch_.data(), count, *act,
                                                      midiScratch2_.data(),
                                                      (int) midiScratch2_.size());
                nd.midi->deliverMidi(p, midiScratch2_.data(), n2);
                if (n2 > 0) rtStoreWord(nd.actMidi, nd.actMidi + (float) n2);
                continue;
            }
        }
        nd.midi->deliverMidi(p, midiScratch_.data(), count);
        if (count > 0)
            rtStoreWord(nd.actMidi, nd.actMidi + (float) count);
    }
}

}
