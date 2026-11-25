// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <cstring>

#include "core/plugins/clap/ClapTransport.h"
#include "gui/plugins/clap/ClapInstance.h"
#include "hum/dsp/DspMath.h"

namespace hum::claphost {

void Instance::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    const juce::ScopedNoDenormals noDenormals;
    const juce::ScopedTryLock hold(getCallbackLock());
    const auto* plugin = session_->plugin();
    if (!hold.isLocked() || isSuspended() || !active_) {
        buffer.clear();
        midi.clear();
        return;
    }
    const Host::AudioThread audio;
    audioRunning_.store(true);
    if (resetPending_.exchange(false) && processing_) {
        plugin->stop_processing(plugin);
        processing_ = false;
        plugin->reset(plugin);
    }
    if (!processing_) processing_ = plugin->start_processing(plugin);
    const int n = juce::jmin(buffer.getNumSamples(), maxBlock_);
    if (!processing_ || n <= 0) {
        buffer.clear();
        midi.clear();
        return;
    }

    inEvents_.clear();
    outEvents_.clear();
    pushParameterChanges(inEvents_);
    pushMidi(inEvents_, midi);
    wireBuffers(buffer, n);

    clap_process_t pr{};
    pr.steady_time = steadyTime_;
    pr.frames_count = (uint32_t) n;
    pr.transport = updateTransport();
    pr.audio_inputs = inBufs_.data();
    pr.audio_outputs = outBufs_.data();
    pr.audio_inputs_count = (uint32_t) inBufs_.size();
    pr.audio_outputs_count = (uint32_t) outBufs_.size();
    pr.in_events = inEvents_.input();
    pr.out_events = outEvents_.output();
    if (plugin->process(plugin, &pr) == CLAP_PROCESS_ERROR) buffer.clear();
    for (int c = ports_.outChannels(); c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
    if (n < buffer.getNumSamples()) buffer.clear(n, buffer.getNumSamples() - n);

    midi.clear();
    readOutputEvents(midi);
    steadyTime_ += n;
}

void Instance::wireBuffers(juce::AudioBuffer<float>& buffer, int numSamples) {
    int channel = 0;
    for (size_t port = 0; port < ports_.ins.size(); ++port) {
        float** first = inPtrs_.data() + channel;
        for (int c = 0; c < ports_.ins[port]; ++c, ++channel) {
            float* copy = inputCopy_.getWritePointer(channel);
            if (channel < buffer.getNumChannels()) std::memcpy(copy, buffer.getReadPointer(channel), sizeof(float) * (size_t) numSamples);
            else std::memset(copy, 0, sizeof(float) * (size_t) numSamples);
            inPtrs_[(size_t) channel] = copy;
        }
        inBufs_[port] = {first, nullptr, (uint32_t) ports_.ins[port], 0, 0};
    }
    channel = 0;
    for (size_t port = 0; port < ports_.outs.size(); ++port) {
        float** first = outPtrs_.data() + channel;
        for (int c = 0; c < ports_.outs[port]; ++c, ++channel)
            outPtrs_[(size_t) channel] = buffer.getWritePointer(juce::jmin(channel, buffer.getNumChannels() - 1));
        outBufs_[port] = {first, nullptr, (uint32_t) ports_.outs[port], 0, 0};
    }
}

const clap_event_transport_t* Instance::updateTransport() {
    auto* head = getPlayHead();
    const auto position = head != nullptr ? head->getPosition() : juce::Optional<juce::AudioPlayHead::PositionInfo>();
    if (!position.hasValue()) return nullptr;
    TransportState t;
    t.tempo = position->getBpm().orFallback(120.0);
    t.beats = position->getPpqPosition().orFallback(0.0);
    t.seconds = position->getTimeInSeconds().orFallback(t.tempo > 0.0 ? t.beats * kSecondsPerMinute / t.tempo : 0.0);
    t.playing = position->getIsPlaying();
    t.looping = position->getIsLooping();
    if (const auto loop = position->getLoopPoints()) {
        t.loopStartBeats = loop->ppqStart;
        t.loopEndBeats = loop->ppqEnd;
    }
    if (const auto sig = position->getTimeSignature()) {
        t.numerator = sig->numerator;
        t.denominator = sig->denominator;
    }
    transport_ = makeTransport(t);
    return &transport_;
}

void Instance::pushParameterChanges(EventList& into) {
    if (!anyPending_.exchange(false, std::memory_order_acquire)) return;
    for (auto* p : params_) {
        double value = 0.0;
        if (!p->takePending(value)) continue;
        clap_event_param_value_t ev{};
        ev.header = {sizeof(ev), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
        ev.param_id = p->id();
        ev.cookie = p->cookie();
        ev.note_id = -1;
        ev.port_index = ev.channel = ev.key = -1;
        ev.value = value;
        if (!into.push(ev.header)) {
            anyPending_.store(true, std::memory_order_release);
            break;
        }
    }
}

void Instance::pushMidi(EventList& into, const juce::MidiBuffer& midi) {
    if (!ports_.noteIn) return;
    for (const auto meta : midi) {
        const uint32_t time = (uint32_t) juce::jlimit(0, maxBlock_ - 1, meta.samplePosition);
        notes_.push(into, meta.data, meta.numBytes, time, 0, ports_.dialect);
    }
}

void Instance::readOutputEvents(juce::MidiBuffer& midi) {
    for (uint32_t i = 0; i < outEvents_.size(); ++i) {
        const auto* ev = outEvents_.at(i);
        if (ev->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
        if (ev->type == CLAP_EVENT_PARAM_VALUE) {
            const auto& value = reinterpret_cast<const clap_event_param_value_t&>(*ev);
            if (const auto it = byId_.find(value.param_id); it != byId_.end()) it->second->showPluginValue(value.value);
            continue;
        }
        uint8_t bytes[3] = {};
        if (const int size = toMidi(*ev, bytes); size > 0) midi.addEvent(bytes, size, (int) ev->time);
    }
}

}
