#include "core/AudioGraph.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <queue>
#include <typeinfo>

#include "core/AdoptSlot.h"
#include "core/MtsTuning.h"
#include "core/PluginNode.h"
#include "core/RtWord.h"

#include "hum/dsp/DspMath.h"

namespace hum {

int AudioGraph::addNode(OrganismPtr c) {
    Node n;
    n.c = std::move(c);
    nodes_.push_back(std::move(n));
    return (int) nodes_.size() - 1;
}

void AudioGraph::connect(int srcNode, int srcOutlet, int dstNode, int dstInlet) {
    cords_.push_back({srcNode, srcOutlet, dstNode, dstInlet});
}

void AudioGraph::connectMidi(int srcNode, int srcPort, int dstNode, int dstPort, int channel) {
    midiCords_.push_back({srcNode, srcPort, dstNode, dstPort, channel});
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
    std::vector<std::vector<int>> succ(n);
    std::vector<int> indeg(n, 0);
    std::vector<std::vector<char>> seen(n, std::vector<char>(n, 0));
    auto addEdge = [&](int src, int dst) {
        if (src == dst) return;
        if (seen[src][dst]) return;
        seen[src][dst] = 1;
        succ[src].push_back(dst);
        indeg[dst]++;
    };
    for (const auto& c : cords_) addEdge(c.srcNode, c.dstNode);
    for (const auto& c : midiCords_) addEdge(c.srcNode, c.dstNode);
    std::queue<int> q;
    for (int i = 0; i < n; ++i) if (indeg[i] == 0) q.push(i);
    order_.clear();
    while (!q.empty()) {
        int u = q.front(); q.pop();
        order_.push_back(u);
        for (int v : succ[u]) if (--indeg[v] == 0) q.push(v);
    }
    hasFeedback_ = (int) order_.size() < n;
    for (int i = 0; i < n; ++i)
        if (std::find(order_.begin(), order_.end(), i) == order_.end())
            order_.push_back(i);
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
    for (int i = 0; i < (int) nodes_.size(); ++i) {
        int maxIn = 0, maxOut = 0;
        for (const auto& c : cords_) {
            if (c.dstNode == i) maxIn = std::max(maxIn, c.dstInlet + 1);
            if (c.srcNode == i) maxOut = std::max(maxOut, c.srcOutlet + 1);
        }
        nodes_[i].c->configureChannels(maxIn, maxOut);
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
                for (const auto& cord : cords_)
                    if (cord.dstNode == i && cord.dstInlet == c) { fed = true; break; }
                if (!fed) nodes_[i].inPtrs[(size_t) c] = nullptr;
            }

        Node& nd = nodes_[i];
        nd.midi = dynamic_cast<MidiNode*>(nd.c.get());
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

void AudioGraph::resolveTunings() {
    const int n = (int) nodes_.size();
    for (auto& nd : nodes_) nd.tuning = nullptr;
    defaultTuning_ = nullptr;

    std::vector<std::vector<int>> succ((size_t) n);
    for (const auto& c : midiCords_)
        if (c.srcNode != c.dstNode) succ[(size_t) c.srcNode].push_back(c.dstNode);

    for (int i = 0; i < n; ++i) {
        auto* tp = nodes_[(size_t) i].tuningProvider;
        if (tp == nullptr) continue;
        if (succ[(size_t) i].empty()) {
            if (defaultTuning_ == nullptr) defaultTuning_ = tp;
            continue;
        }
        std::vector<char> seen((size_t) n, 0);
        std::vector<int> stack = succ[(size_t) i];
        while (!stack.empty()) {
            const int v = stack.back();
            stack.pop_back();
            if (seen[(size_t) v]) continue;
            seen[(size_t) v] = 1;
            Node& d = nodes_[(size_t) v];
            if (d.tuningProvider != nullptr) continue;
            if (d.tuning != nullptr) continue;
            d.tuning = &tp->tuning();
            for (int w : succ[(size_t) v]) stack.push_back(w);
        }
    }
    transport_.setActiveTuning(defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr);
}

void AudioGraph::pumpPluginTunings() {
    const Tuning* def = defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr;
    static const Tuning kStandard;
    for (auto& nd : nodes_) {
        if (nd.plugin == nullptr) continue;
        const Tuning& active = nd.tuning != nullptr ? *nd.tuning
                             : def != nullptr       ? *def
                                                    : kStandard;
        if (active.pluginDelivery() == Tuning::kDeliverBend) continue;
        if (active.pluginDelivery() == Tuning::kDeliverAuto && nd.bendVerdict) continue;
        if (active == nd.sentTuning) continue;
        for (const auto& m : mtsRetuneMessages(active)) nd.plugin->queueMidiMessage(m);
        nd.sentTuning = active;
    }
}

std::vector<std::string> AudioGraph::pluginsNeedingVerdict() const {
    std::vector<std::string> out;
    const Tuning* def = defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr;
    for (const auto& nd : nodes_) {
        if (nd.plugin == nullptr || nd.verdictKnown) continue;
        const Tuning* act = nd.tuning != nullptr ? nd.tuning : def;
        if (act == nullptr || act->isStandard()) continue;
        if (act->pluginDelivery() != Tuning::kDeliverAuto) continue;
        const auto& cls = nd.plugin->classRaw();
        bool dup = false;
        for (const auto& c : out) dup |= c == cls;
        if (!dup) out.push_back(cls);
    }
    return out;
}

void AudioGraph::setBendVerdict(const std::string& classRaw, bool needsBend) {
    for (auto& nd : nodes_) {
        if (nd.plugin == nullptr || nd.plugin->classRaw() != classRaw) continue;
        nd.bendVerdict = needsBend;
        nd.verdictKnown = true;
        nd.sentTuning = Tuning();
    }
}

void AudioGraph::computeLatencyCompensation() {
    const int n = (int) nodes_.size();
    cordDelay_.assign(cords_.size(), CordDelay{});
    for (int i = 0; i < n; ++i) {
        auto* lr = dynamic_cast<LatencyReporting*>(nodes_[i].c.get());
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
        for (const auto& c : cords_) {
            if (c.dstNode != node) continue;
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
        n.plugin = dynamic_cast<PluginNode*>(n.c.get());
    }
    resolveTunings();
}

void AudioGraph::applyAutomation() {
    if (autoLanes_.empty() || !transport_.playing()) { lastAutoBeat_ = transport_.beats(); return; }
    const double beat = transport_.beats();
    for (const auto& lane : autoLanes_) {
        if (lane.mute || lane.points.empty()) continue;
        if (lane.suspended) continue;
        if (lane.node == AutoLane::kTempoNode) {
            if (!externalTempo_.load(std::memory_order_relaxed))
                transport_.setTempo(std::clamp(evaluateEnvelope(lane.points, beat),
                                               20.0, 999.0));
            continue;
        }
        if (lane.node < 0 || lane.node >= (int) nodes_.size()) continue;
        auto* p = nodes_[lane.node].c->params.byName(lane.param);
        if (!p) continue;
        switch (lane.kind) {
            case AutoKind::Range: {
                const double lo = evaluateEnvelope(lane.points, beat);
                const double hi = evaluateEnvelopeMax(lane.points, beat);
                rtStoreWord(p->value, lo);
                rtStoreWord(p->rangeMin, lo);
                rtStoreWord(p->rangeMax, hi);
                rtStoreWord(p->isRange, true);
                break;
            }
            case AutoKind::Trigger: {
                const bool fired = triggerFiredBetween(lane.points, lastAutoBeat_, beat);
                const double v = fired ? lane.pulse : lane.rest;
                rtStoreWord(p->value, v);
                rtStoreWord(p->rangeMin, v);
                rtStoreWord(p->rangeMax, v);
                break;
            }
            case AutoKind::Double:
            default: {
                const double v = evaluateEnvelope(lane.points, beat);
                rtStoreWord(p->value, v);
                rtStoreWord(p->rangeMin, v);
                rtStoreWord(p->rangeMax, v);
                break;
            }
        }
    }
    lastAutoBeat_ = beat;
}

void AudioGraph::processBlock(int numSamples) {
    if (!externalTempo_.load(std::memory_order_relaxed))
        for (int node : order_) {
            const double t = nodes_[node].c->masterTempo();
            if (t > 0.0) { transport_.setTempo(t); break; }
        }
    applyAutomation();

    for (auto& nd : nodes_)
        if (nd.tuningProvider != nullptr) nd.tuningProvider->refreshTuning();

    const Tuning* defaultTuning =
        defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr;
    for (int node : order_) {
        Node& nd = nodes_[node];

        for (auto& ch : nd.inBuf) std::fill(ch.begin(), ch.begin() + numSamples, 0.0f);
        for (size_t ci = 0; ci < cords_.size(); ++ci) {
            const Cord& c = cords_[ci];
            if (c.dstNode != node) continue;
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

static bool cordPasses(const AudioGraph::MidiCord& mc, const MidiEvent& e) {
    if (mc.channel <= 0) return true;
    const unsigned char status = e.data[0];
    if (status < 0x80 || status >= 0xF0) return true;
    return (status & 0x0F) == mc.channel - 1;
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

int AudioGraph::gatherMidiFor(int node, int port) {
    int count = 0;
    for (const auto& mc : midiCords_) {
        if (mc.dstNode != node || mc.dstPort != port) continue;
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
            e.data[0] = 0xB0;
            e.data[1] = (unsigned char) std::clamp(ed.cc, 0, kMidiMax);
            e.data[2] = (unsigned char) std::clamp(ed.vel, 0, kMidiMax);
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
