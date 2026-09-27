// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/graph/AudioGraph.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstddef>
#include <queue>
#include <typeinfo>
#include "core/graph/AdoptSlot.h"
#include "core/graph/InletNames.h"
#include "core/tuning/MtsTuning.h"
#include "core/plugins/PluginNode.h"
#include "core/graph/RtWord.h"
#include "core/params/SpanMove.h"
#include "hum/Swing.h"
#include "hum/dsp/DspMath.h"

namespace hum {

void AudioGraph::setAutomation(std::vector<AutoLane> lanes) {
    for (auto& lane : lanes)
        lane.paramSlot = lane.node >= 0 && lane.node < (int) nodes_.size() && nodes_[(size_t) lane.node].c
                             ? nodes_[(size_t) lane.node].c->params.slotOf(lane.param) : -1;
    autoLanes_ = std::move(lanes);
}

void AudioGraph::setMeterMap(std::vector<MeterChange> changes) {
    if (changes.empty()) changes.push_back({0.0, Meter{}});
    changes.front().beat = 0.0;
    meterChanges_ = std::move(changes);
    transportExt_.meterChanges = meterChanges_.data();
    transportExt_.meterCount = (std::int32_t) meterChanges_.size();
    transport_.setExt(&transportExt_);
    transport_.setBeatsPerBar(transport_.meter().quarterNotesPerBar());
}

void AudioGraph::setModRoutes(std::vector<ModRoute> routes) {
    for (auto& r : routes) {
        const bool ok = r.srcNode >= 0 && r.srcNode < (int) nodes_.size() && nodes_[(size_t) r.srcNode].c
                     && r.dstNode >= 0 && r.dstNode < (int) nodes_.size() && nodes_[(size_t) r.dstNode].c;
        if (!ok) { r.dstSlot = -1; continue; }
        r.dstSlot = nodes_[(size_t) r.dstNode].c->params.slotOf(r.dstParam);
        if (r.srcIsParam) {
            r.srcSlot = nodes_[(size_t) r.srcNode].c->params.slotOf(r.srcValue);
            r.src = nullptr;
        } else {
            r.src = dynamic_cast<const ControlSource*>(nodes_[(size_t) r.srcNode].c.get());
        }
    }
    modRoutes_ = std::move(routes);
    rebindNamedInlets();
    routesInto_.assign(nodes_.size(), {});
    for (int i = 0; i < (int) modRoutes_.size(); ++i) {
        const auto& r = modRoutes_[(size_t) i];
        if (r.dstSlot >= 0 && r.dstNode >= 0 && r.dstNode < (int) nodes_.size())
            routesInto_[(size_t) r.dstNode].push_back(i);
    }
    if (prepared_) {
        computeOrder();
        computeLatencyCompensation();
    }
}

void AudioGraph::rebindNamedInlets() {
    tapsInto_.assign(nodes_.size(), {});
    for (int node = 0; node < (int) nodes_.size(); ++node) {
        auto* sink = nodes_[(size_t) node].c ? dynamic_cast<NamedInlet*>(nodes_[(size_t) node].c.get()) : nullptr;
        if (sink == nullptr) continue;
        auto& params = nodes_[(size_t) node].c->params;
        char names[NamedInlet::kMaxNames][NamedInlet::kNameChars] = {};
        int count = 0;
        for (const auto& r : modRoutes_) {
            if (r.dstNode != node || r.dstParam != sink->namedInletParam()) continue;
            if (r.srcNode < 0 || r.srcNode >= (int) nodes_.size() || !nodes_[(size_t) r.srcNode].c) continue;
            const auto* tagged = dynamic_cast<const Tagged*>(nodes_[(size_t) r.srcNode].c.get());
            const auto* src = dynamic_cast<const ControlSource*>(nodes_[(size_t) r.srcNode].c.get());
            if (tagged == nullptr || src == nullptr) continue;
            const auto tag = tagged->tag();
            if (tag.empty()) continue;
            const auto name = inletNameFor(tag);
            if (const char* knob = sink->knobForName(name.c_str())) {
                if (auto* p = params.slot(params.slotOf(knob))) tapsInto_[(size_t) node].push_back({src, sink, -1, p});
                continue;
            }
            if (count >= NamedInlet::kMaxNames || !sink->readsName(name.c_str())) continue;
            bool seen = false;
            for (int k = 0; k < count && !seen; ++k) seen = name == names[k];
            if (seen) continue;
            std::snprintf(names[count], NamedInlet::kNameChars, "%s", name.c_str());
            tapsInto_[(size_t) node].push_back({src, sink, count, nullptr});
            ++count;
        }
        sink->setInletNames(names, count);
    }
}

void AudioGraph::applyNamedTaps(int node) {
    if (node < 0 || node >= (int) tapsInto_.size()) return;
    for (const auto& tap : tapsInto_[(size_t) node]) {
        ControlSource::ControlVal vals[4];
        if (tap.src->controlValues(vals, 4) <= 0) continue;
        if (tap.knob != nullptr) rtStoreWord(tap.knob->value, (double) vals[0].value);
        else tap.sink->setInletValue(tap.slot, vals[0].value);
    }
}

void AudioGraph::applyModRoutes(int numSamples) {
    if (modRoutes_.empty()) return;
    const double sr = transport_.sampleRate() > 0.0 ? transport_.sampleRate() : kDefaultSampleRate;
    const double dt = (double) numSamples / sr;
    for (auto& r : modRoutes_) applyModRoute(r, dt);
}

void AudioGraph::applyModRoutesInto(int node, int numSamples) {
    if (modRoutes_.empty() || node < 0 || node >= (int) routesInto_.size()) return;
    const auto& list = routesInto_[(size_t) node];
    if (list.empty()) return;
    const double sr = transport_.sampleRate() > 0.0 ? transport_.sampleRate() : kDefaultSampleRate;
    const double dt = (double) numSamples / sr;
    for (int i : list) applyModRoute(modRoutes_[(size_t) i], dt);
}

namespace {

void landOn(Parameter& p, RangeEnd end, double at, double floorAt, double ceilingAt) {
    if (!rtLoadWord(p.isRange)) {
        rtStoreWord(p.value, at);
        return;
    }
    double lo = rtLoadWord(p.rangeMin), hi = rtLoadWord(p.rangeMax);
    moveSpan(end, at, floorAt, ceilingAt, lo, hi);
    rtStoreWord(p.value, lo);
    rtStoreWord(p.rangeMin, lo);
    rtStoreWord(p.rangeMax, hi);
}

}

void AudioGraph::applyModRoute(ModRoute& r, double dt) {
    {
        if (r.dstSlot < 0) return;
        double raw = 0.0;
        float v = 0.0f;
        if (r.srcIsParam) {
            if (r.srcSlot < 0) return;
            const auto* sp = nodes_[(size_t) r.srcNode].c->params.slot(r.srcSlot);
            if (sp == nullptr || r.srcMax <= r.srcMin) return;
            raw = rtLoadWord(sp->value);
            v = (float) std::clamp((raw - r.srcMin) / (r.srcMax - r.srcMin), 0.0, 1.0);
        } else {
            if (r.src == nullptr) return;
            ControlSource::ControlVal vals[32];
            const int n = r.src->controlValues(vals, 32);
            bool found = false;
            for (int i = 0; i < n && !found; ++i)
                if (std::strcmp(vals[i].name, r.srcValue.c_str()) == 0) {
                    raw = vals[i].value;
                    v = ControlSource::unitOf(vals[i]);
                    found = true;
                }
            if (!found) return;
        }
        auto* p = nodes_[(size_t) r.dstNode].c->params.slot(r.dstSlot);
        if (p == nullptr) return;
        if (r.carry) {
            landOn(*p, r.dstEnd, r.dstHi > r.dstLo ? std::clamp(raw, r.dstLo, r.dstHi) : raw,
                   r.dstLo, r.dstHi);
            return;
        }
        const double shaped = advanceControlShape(r.shape, r.state, v, dt);
        if (shaped < 0.0) return;
        landOn(*p, r.dstEnd, shapedToRange(r.shape, r.min, r.max, shaped), r.min, r.max);
    }
}

void AudioGraph::applyAutomation() {
    if (autoLanes_.empty() || !transport_.playing()) { lastAutoBeat_ = transport_.beats(); return; }
    const double beat = transport_.beats();
    for (auto& lane : autoLanes_) {
        if (lane.mute || lane.points.empty()) continue;
        if (lane.suspended) continue;
        if (lane.node == AutoLane::kTempoNode) {
            if (!externalTempo_.load(std::memory_order_relaxed))
                transport_.setTempo(std::clamp(evaluateEnvelope(lane.points, beat),
                                               kMinTempoBpm, kMaxTempoBpm));
            continue;
        }
        if (lane.node == AutoLane::kGrooveNode || lane.node == AutoLane::kGrooveGridNode) {
            auto groove = transport_.groove();
            if (lane.node == AutoLane::kGrooveNode)
                groove.amount = std::clamp(evaluateEnvelope(lane.points, beat), 0.0, 1.0);
            else
                groove.gridTicks = swing::gridTicksAt(std::lround(evaluateHeld(lane.points, beat)));
            transport_.setGroove(groove);
            continue;
        }
        if (lane.node < 0 || lane.node >= (int) nodes_.size()) continue;
        auto& ps = nodes_[(size_t) lane.node].c->params;
        if (lane.paramSlot < 0) lane.paramSlot = ps.slotOf(lane.param);
        auto* p = ps.slot(lane.paramSlot);
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
            case AutoKind::Step: {
                const double v = evaluateHeld(lane.points, beat);
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

}
