// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/HostedPlugin.h"

#include <algorithm>
#include <cstring>

#include "core/plugins/BridgedPlugin.h"
#include "core/plugins/PluginHost.h"

#include "hum/dsp/DspMath.h"

namespace hum {

HostedPlugin::HostedPlugin(std::unique_ptr<juce::AudioPluginInstance> instance,
                           std::string classRaw)
    : instance_(std::move(instance)), classRaw_(std::move(classRaw)) {
    ins_ = std::max(0, instance_->getTotalNumInputChannels());
    outs_ = std::max(1, instance_->getTotalNumOutputChannels());

    for (const auto& d : PluginHost::instance().schemaFor(classRaw_)) {
        Parameter p;
        p.index = (int) params.all().size();
        p.name = d.name;
        p.type = "double";
        p.value = d.def;
        params.add(p);
    }
}

HostedPlugin::~HostedPlugin() {
    if (!instance_) return;
    instance_->releaseResources();
    if (shouldLeakInstance && shouldLeakInstance(classRaw_)) {
        (void) instance_.release();
        return;
    }
    auto guard = disposeGuard ? disposeGuard(classRaw_) : nullptr;
    instance_.reset();
}

void HostedPlugin::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    instance_->setPlayHead(&playHead_);
    instance_->setRateAndBufferSizeDetails(sampleRate, maxBlock);
    instance_->prepareToPlay(sampleRate, maxBlock);
    latency_ = std::max(0, instance_->getLatencySamples());
    scratch_.setSize(std::max(1, std::max(ins_, outs_)), std::max(1, maxBlock));
    midi_.ensureSize(4096);
    pendingMidi_.ensureSize(4096);
    staged_.assign(MidiNode::kMaxMidiEventsPerBlock, MidiEvent{});
    stagedCount_ = 0;
    lastSentParams_.assign(instance_->getParameters().size(), -1.0f);
}

void HostedPlugin::reset() {
    if (instance_) instance_->reset();
}

void HostedPlugin::process(const float* const* in, int numIn,
                           float* const* out, int numOut,
                           int numSamples, const Transport& transport) {
    if (instance_) {
        const int now = std::max(0, instance_->getLatencySamples());
        if (now != latency_) {
            latency_ = now;
            latencyChanged_.store(true, std::memory_order_relaxed);
        }
    }
    const int chans = scratch_.getNumChannels();
    const int n = std::min(numSamples, scratch_.getNumSamples());

    for (int c = 0; c < chans; ++c) {
        float* dst = scratch_.getWritePointer(c);
        if (c < numIn && in && in[c]) std::memcpy(dst, in[c], sizeof(float) * (size_t) n);
        else std::memset(dst, 0, sizeof(float) * (size_t) n);
    }

    const auto& pluginParams = instance_->getParameters();
    const int np = std::min((int) pluginParams.size(), (int) lastSentParams_.size());
    for (const auto& p : params.all()) {
        if (p.index < 0 || p.index >= np) continue;
        const float v = (float) std::clamp(p.value, 0.0, 1.0);
        if (v != lastSentParams_[(size_t) p.index]) {
            pluginParams[(size_t) p.index]->setValue(v);
            lastSentParams_[(size_t) p.index] = v;
        }
    }
    pushedOnce_.store(true, std::memory_order_relaxed);

    playHead_.bpm.store(transport.tempo());
    playHead_.ppq.store(transport.beats());
    playHead_.playing.store(transport.playing());

    juce::AudioBuffer<float> view(scratch_.getArrayOfWritePointers(), chans, n);
    midi_.clear();
    for (int i = 0; i < stagedCount_; ++i)
        midi_.addEvent(staged_[(size_t) i].data, staged_[(size_t) i].size,
                       staged_[(size_t) i].sampleOffset);
    stagedCount_ = 0;
    {
        const juce::ScopedTryLock ml(midiLock_);
        if (ml.isLocked() && !pendingMidi_.isEmpty()) {
            if (midi_.isEmpty()) midi_.swapWith(pendingMidi_);
            else { midi_.addEvents(pendingMidi_, 0, -1, 0); pendingMidi_.clear(); }
        }
    }
    instance_->processBlock(view, midi_);

    for (int c = 0; c < numOut; ++c) {
        if (c < chans) std::memcpy(out[c], scratch_.getReadPointer(c), sizeof(float) * (size_t) n);
        else std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    }
}

void HostedPlugin::deliverMidi(int, const MidiEvent* events, int count) {
    stagedCount_ = std::min(count, (int) staged_.size());
    for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
}

int HostedPlugin::collectMidi(int, MidiEvent* out, int capacity) {
    int n = 0;
    for (const auto meta : midi_) {
        if (n >= capacity) break;
        if (meta.numBytes < 1 || meta.numBytes > 3) continue;
        MidiEvent e;
        e.sampleOffset = meta.samplePosition;
        for (int i = 0; i < meta.numBytes; ++i) e.data[i] = meta.data[i];
        e.size = meta.numBytes;
        out[n++] = e;
    }
    return n;
}

std::string HostedPlugin::getStateBase64() const {
    juce::MemoryBlock mb;
    instance_->getStateInformation(mb);
    return juce::Base64::toBase64(mb.getData(), mb.getSize()).toStdString();
}

void HostedPlugin::setStateBase64(const std::string& base64) {
    juce::MemoryOutputStream mo;
    if (juce::Base64::convertFromBase64(mo, juce::String(base64)) && mo.getDataSize() > 0)
        instance_->setStateInformation(mo.getData(), (int) mo.getDataSize());
}

OrganismPtr PluginHost::createOrganism(const std::string& classRaw) {
    auto desc = instance().descriptionFor(classRaw);
    if (!desc) return nullptr;
    if (instance().shouldBridge(classRaw)) {
        auto bp = std::make_unique<BridgedPlugin>(classRaw, *desc, instance().bridgeExe());
        if (bp->launched()) return bp;
    }
    std::string err;
    auto inst = instance().createInstance(*desc, kDefaultSampleRate, 512, err);
    if (!inst) return nullptr;
    return std::make_unique<HostedPlugin>(std::move(inst), classRaw);
}

}
