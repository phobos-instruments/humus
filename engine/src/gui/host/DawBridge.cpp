// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/DawBridge.h"

#include <algorithm>
#include <cmath>

#include "core/graph/AudioGraph.h"
#include "core/graph/RtWord.h"
#include "core/packs/ClassString.h"
#include "core/params/ParamSchema.h"
#include "core/plugins/PluginNode.h"
#include "io/PatchDocument.h"

namespace hum {

namespace {

constexpr double kJumpBeats = 0.1;

TappedMidi tapped(const unsigned char* raw, int size, int port) {
    return {0.0, port, raw[0], size > 1 ? raw[1] : (unsigned char) 0, size > 2 ? raw[2] : (unsigned char) 0};
}

}

void DawBridge::applyClock(Transport& t, bool playIntent, int numSamples) {
    if (!clock_.known) return;
    if (clock_.bpm > 0.0) t.setTempo(clock_.bpm);
    if (clock_.playing) {
        const bool jumped = !wasPlaying_ || lastPpq_ < 0.0
                            || std::abs(clock_.ppq - (lastPpq_ + (double) lastBlock_ / t.samplesPerBeat())) > kJumpBeats;
        if (clock_.ppq >= 0.0 && jumped) t.setBeatPosition(clock_.ppq);
        lastPpq_ = clock_.ppq;
        lastBlock_ = numSamples;
        t.setPlaying(true);
    } else {
        if (wasPlaying_) t.setPlaying(playIntent);
        lastPpq_ = -1.0;
    }
    wasPlaying_ = clock_.playing;
}

void DawBridge::rebuild(AudioGraph* graph, const PatchDocumentModel& model) {
    resolveTargets(graph, model);
    collectPorts(graph, model);
}

void DawBridge::collectPorts(AudioGraph* graph, const PatchDocumentModel& model) {
    ports_ = {};
    if (graph == nullptr) return;
    ports_ = graphmidi::findPorts(*graph);
    for (const auto& cm : model.organisms) {
        if (!isPluginKind(cm.kind)) continue;
        if (auto* node = dynamic_cast<PluginNode*>(graph->find(cm.name)))
            ports_.hosted.push_back({node, cm.name, cm.midiReceiveMode, cm.midiReceiveChannel});
    }
}

void DawBridge::deliverMidi() {
    if (in_ == nullptr) return;
    graphmidi::deliver(*in_, ports_);
    for (const auto meta : *in_) {
        if (meta.numBytes < 1 || meta.numBytes > 3 || meta.data[0] == 0xf8 || meta.data[0] == 0xfe) continue;
        seen_->push(tapped(meta.data, meta.numBytes, 1));
    }
}

void DawBridge::collectMidi() {
    if (out_ == nullptr) return;
    graphmidi::collect(ports_, *out_);
    toSend_->drainEach([this](const TappedMidi& t) {
        const unsigned char bytes[3] = {t.status, t.d1, t.d2};
        out_->addEvent(bytes, juce::MidiMessage::getMessageLengthFromFirstByte(t.status), 0);
    });
}

void DawBridge::drainSeen(const std::function<void(const juce::MidiMessage&)>& noteControl) {
    seen_->drainEach([&](const TappedMidi& t) {
        const unsigned char bytes[3] = {t.status, t.d1, t.d2};
        noteControl(juce::MidiMessage(bytes, juce::MidiMessage::getMessageLengthFromFirstByte(t.status)));
    });
}

void DawBridge::send(const juce::MidiMessage& m) {
    if (m.getRawDataSize() < 1 || m.getRawDataSize() > 3) return;
    toSend_->push(tapped(m.getRawData(), m.getRawDataSize(), 0));
}

std::vector<DawMacroTarget> DawBridge::targets(int slot) const {
    std::vector<DawMacroTarget> out;
    if (slot < 0 || slot >= kControls) return out;
    for (const auto& t : targets_[(size_t) slot]) out.push_back(t.spec);
    return out;
}

int DawBridge::slotFor(const std::string& organism, const std::string& param) const {
    for (int slot = 0; slot < kControls; ++slot)
        for (const auto& t : targets_[(size_t) slot])
            if (t.spec.is(organism, param)) return slot;
    return -1;
}

bool DawBridge::add(int slot, const DawMacroTarget& target, AudioGraph* graph, const PatchDocumentModel& model) {
    if (slot < 0 || slot >= kControls || target.organism.empty()) return false;
    remove(target.organism, target.param);
    auto& list = targets_[(size_t) slot];
    const bool first = list.empty();
    list.push_back({target});
    resolveTargets(graph, model);
    return first;
}

void DawBridge::setTargets(int slot, const std::vector<DawMacroTarget>& targets, AudioGraph* graph,
                           const PatchDocumentModel& model) {
    if (slot < 0 || slot >= kControls) return;
    targets_[(size_t) slot].clear();
    for (const auto& t : targets)
        if (!t.organism.empty()) {
            remove(t.organism, t.param);
            targets_[(size_t) slot].push_back({t});
        }
    resolveTargets(graph, model);
}

void DawBridge::remove(const std::string& organism, const std::string& param) {
    for (size_t slot = 0; slot < targets_.size(); ++slot) {
        auto& list = targets_[slot];
        const auto before = list.size();
        list.erase(std::remove_if(list.begin(), list.end(), [&](const Target& t) { return t.spec.is(organism, param); }),
                   list.end());
        if (list.size() != before) applied_[slot] = -1.0f;
    }
}

void DawBridge::setRange(const std::string& organism, const std::string& param, double from, double to) {
    for (size_t slot = 0; slot < targets_.size(); ++slot)
        for (auto& t : targets_[slot])
            if (t.spec.is(organism, param)) {
                t.spec.from = std::clamp(from, 0.0, 1.0);
                t.spec.to = std::clamp(to, 0.0, 1.0);
                applied_[slot] = -1.0f;
            }
}

void DawBridge::noteMappedAt(int slot, double value) {
    if (slot < 0 || slot >= kControls || targets_[(size_t) slot].empty()) return;
    const auto& t = targets_[(size_t) slot].front();
    if (t.hi <= t.lo) return;
    const double at = std::clamp((value - t.lo) / (t.hi - t.lo), 0.0, 1.0);
    const double span = t.spec.to - t.spec.from;
    const auto now = (float) (span == 0.0 ? 0.0 : std::clamp((at - t.spec.from) / span, 0.0, 1.0));
    values_[(size_t) slot] = now;
    if (onControlMapped) onControlMapped(slot, now);
}

void DawBridge::resolveTargets(AudioGraph* graph, const PatchDocumentModel& model) {
    for (auto& list : targets_) {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [&](const Target& t) { return model.byName(t.spec.organism) == nullptr; }),
                   list.end());
        for (auto& t : list) {
            t.live = nullptr;
            if (graph == nullptr) continue;
            auto* c = graph->find(t.spec.organism);
            auto* p = c != nullptr ? c->params.byName(t.spec.param) : nullptr;
            if (p == nullptr) continue;
            t.lo = 0.0;
            t.hi = 1.0;
            t.stepped = false;
            if (const auto* cm = model.byName(t.spec.organism))
                for (const auto& d : schemaFor(cm->displayClass))
                    if (d.name == t.spec.param) {
                        t.lo = d.min;
                        t.hi = d.max;
                        t.stepped = d.isInt || d.isEnum || d.isBool;
                    }
            t.live = p;
        }
    }
    applied_.fill(-1.0f);
}

void DawBridge::applyControls() {
    for (size_t slot = 0; slot < targets_.size(); ++slot) {
        if (values_[slot] == applied_[slot]) continue;
        for (const auto& t : targets_[slot]) {
            if (t.live == nullptr) continue;
            const double at = t.spec.from + (double) values_[slot] * (t.spec.to - t.spec.from);
            double v = t.lo + at * (t.hi - t.lo);
            if (t.stepped) v = std::round(v);
            rtStoreWord(t.live->value, v);
            rtStoreWord(t.live->rangeMin, v);
            rtStoreWord(t.live->rangeMax, v);
        }
        applied_[slot] = values_[slot];
    }
}

}
