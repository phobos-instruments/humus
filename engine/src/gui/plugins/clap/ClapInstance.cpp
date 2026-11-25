// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/clap/ClapInstance.h"

#include <optional>
#include <utility>

#include "core/plugins/clap/ClapStreams.h"
#include "gui/plugins/clap/ClapEditor.h"

namespace hum::claphost {

namespace {
juce::AudioChannelSet channelSet(int channels) {
    if (channels == 1) return juce::AudioChannelSet::mono();
    if (channels == 2) return juce::AudioChannelSet::stereo();
    return juce::AudioChannelSet::discreteChannels(channels);
}
}

juce::AudioProcessor::BusesProperties Instance::busesFor(const PortLayout& ports) {
    BusesProperties buses;
    for (size_t i = 0; i < ports.ins.size(); ++i)
        buses = buses.withInput(i == 0 ? "Input" : "Input " + juce::String((int) i + 1), channelSet(ports.ins[i]), true);
    for (size_t i = 0; i < ports.outs.size(); ++i)
        buses = buses.withOutput(i == 0 ? "Output" : "Output " + juce::String((int) i + 1), channelSet(ports.outs[i]), true);
    return buses;
}

Instance::Instance(std::unique_ptr<Session> session, const juce::PluginDescription& description)
    : AudioPluginInstance(busesFor(session->ports())), session_(std::move(session)), description_(description),
      ports_(session_->ports()) {
    session_->setOwner(this);
    buildParameters();
}

Instance::~Instance() {
    {
        const juce::ScopedLock hold(getCallbackLock());
        deactivate();
    }
    session_->setOwner(nullptr);
}

void Instance::buildParameters() {
    const auto* params = session_->ext().params;
    if (params == nullptr) return;
    const auto* plugin = session_->plugin();
    auto text = [plugin, params](clap_id id, double v) {
        char out[256] = {};
        return params->value_to_text(plugin, id, v, out, sizeof(out)) ? juce::String::fromUTF8(out) : juce::String();
    };
    auto parse = [plugin, params](clap_id id, const juce::String& s, double& v) {
        return params->text_to_value(plugin, id, s.toRawUTF8(), &v);
    };
    const uint32_t n = params->count(plugin);
    for (uint32_t i = 0; i < n; ++i) {
        clap_param_info_t info{};
        if (!params->get_info(plugin, i, &info) || (info.flags & CLAP_PARAM_IS_HIDDEN) != 0) continue;
        double value = info.default_value;
        params->get_value(plugin, info.id, &value);
        auto p = std::make_unique<ClapParameter>(info, value, text, parse, anyPending_, [this] { scheduleFlush(); });
        params_.push_back(p.get());
        byId_[info.id] = p.get();
        addHostedParameter(std::move(p));
    }
}

void Instance::refreshParameterValues() {
    const auto* params = session_->ext().params;
    if (params == nullptr) return;
    for (auto* p : params_) {
        double value = 0.0;
        if (!params->get_value(session_->plugin(), p->id(), &value)) continue;
        p->showPluginValue(value);
        p->sendValueChangedMessageToListeners(p->getValue());
    }
}

void Instance::prepareToPlay(double sampleRate, int maxBlock) {
    const juce::ScopedLock hold(getCallbackLock());
    if (active_ && (sampleRate != sampleRate_ || maxBlock > maxBlock_)) deactivate();
    sampleRate_ = sampleRate;
    maxBlock_ = juce::jmax(1, maxBlock);
    allocateBuffers();
    activate();
}

void Instance::allocateBuffers() {
    inputCopy_.setSize(juce::jmax(1, ports_.inChannels()), maxBlock_);
    inPtrs_.assign((size_t) juce::jmax(1, ports_.inChannels()), nullptr);
    outPtrs_.assign((size_t) juce::jmax(1, ports_.outChannels()), nullptr);
    inBufs_.assign(ports_.ins.size(), clap_audio_buffer_t{});
    outBufs_.assign(ports_.outs.size(), clap_audio_buffer_t{});
}

void Instance::releaseResources() {
    const juce::ScopedLock hold(getCallbackLock());
    deactivate();
}

void Instance::activate() {
    if (active_) return;
    const auto* plugin = session_->plugin();
    active_ = plugin->activate(plugin, sampleRate_, 1, (uint32_t) maxBlock_);
    notes_.reset();
    if (active_ && session_->ext().latency != nullptr) setLatencySamples((int) session_->ext().latency->get(plugin));
}

void Instance::stopProcessingLocked() {
    if (!processing_) return;
    const Host::AudioThread actingAsAudio;
    session_->plugin()->stop_processing(session_->plugin());
    processing_ = false;
}

void Instance::deactivate() {
    if (!active_) return;
    stopProcessingLocked();
    active_ = false;
    audioRunning_.store(false);
    session_->plugin()->deactivate(session_->plugin());
}

void Instance::setNonRealtime(bool offline) noexcept {
    AudioPluginInstance::setNonRealtime(offline);
    if (const auto* render = session_->ext().render; render != nullptr && juce::MessageManager::existsAndIsCurrentThread())
        render->set(session_->plugin(), offline ? CLAP_RENDER_OFFLINE : CLAP_RENDER_REALTIME);
}

double Instance::getTailLengthSeconds() const {
    const auto* tail = session_->ext().tail;
    if (tail == nullptr || sampleRate_ <= 0.0) return 0.0;
    const uint32_t samples = tail->get(session_->plugin());
    return samples == UINT32_MAX ? 3600.0 : samples / sampleRate_;
}

juce::AudioProcessorEditor* Instance::createEditor() {
    return hasEditor() ? new Editor(*this) : nullptr;
}

void Instance::getStateInformation(juce::MemoryBlock& dest) {
    dest.reset();
    const auto* state = session_->ext().state;
    WriteStream out;
    if (state != nullptr && state->save(session_->plugin(), out.stream()))
        dest.append(out.bytes().data(), out.bytes().size());
}

void Instance::setStateInformation(const void* data, int size) {
    const auto* state = session_->ext().state;
    if (state == nullptr || data == nullptr || size <= 0) return;
    ReadStream in(data, (size_t) size);
    if (state->load(session_->plugin(), in.stream())) refreshParameterValues();
}

void Instance::scheduleFlush() {
    if (audioRunning_.load() || flushPending_.exchange(true)) return;
    session_->onMessageThread([this] {
        flushPending_.store(false);
        if (!audioRunning_.load()) flushOnMessageThread();
    });
}

void Instance::flushOnMessageThread() {
    const auto* params = session_->ext().params;
    if (params == nullptr) return;
    const juce::ScopedLock hold(getCallbackLock());
    inEvents_.clear();
    outEvents_.clear();
    pushParameterChanges(inEvents_);
    std::optional<Host::AudioThread> actingAsAudio;
    if (active_) actingAsAudio.emplace();
    params->flush(session_->plugin(), inEvents_.input(), outEvents_.output());
    juce::MidiBuffer unused;
    readOutputEvents(unused);
}

void Instance::restartRequested() {
    const juce::ScopedLock hold(getCallbackLock());
    const bool wasActive = active_;
    deactivate();
    adoptPorts();
    if (!wasActive) return;
    allocateBuffers();
    activate();
}

void Instance::portsChanged() {
    const juce::ScopedLock hold(getCallbackLock());
    if (active_) restartRequested();
    else adoptPorts();
}

void Instance::adoptPorts() {
    const auto now = session_->ports();
    if (now.ins != ports_.ins) reshapeBuses(true, now.ins);
    if (now.outs != ports_.outs) reshapeBuses(false, now.outs);
    ports_ = now;
}

void Instance::reshapeBuses(bool isInput, const std::vector<int>& channels) {
    while (getBusCount(isInput) < (int) channels.size() && addBus(isInput)) {}
    while (getBusCount(isInput) > (int) channels.size() && removeBus(isInput)) {}
    for (int i = 0; i < getBusCount(isInput) && i < (int) channels.size(); ++i) {
        setChannelLayoutOfBus(isInput, i, channelSet(channels[(size_t) i]));
        if (auto* bus = getBus(isInput, i)) bus->enable(true);
    }
}

bool Instance::canApplyBusCountChange(bool isInput, bool adding, BusProperties& props) {
    if (!adding) return true;
    props.busName = (isInput ? "Input " : "Output ") + juce::String(getBusCount(isInput) + 1);
    props.defaultLayout = juce::AudioChannelSet::stereo();
    props.isActivatedByDefault = true;
    return true;
}

void Instance::paramsChanged(uint32_t) { refreshParameterValues(); }

void Instance::latencyChanged() {
    if (active_ && session_->ext().latency != nullptr) setLatencySamples((int) session_->ext().latency->get(session_->plugin()));
}

void Instance::flushRequested() { scheduleFlush(); }

bool Instance::resizeRequested(uint32_t w, uint32_t h) { return onResizeRequest && onResizeRequest(w, h); }

}
