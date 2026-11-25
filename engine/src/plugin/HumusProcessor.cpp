// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin/HumusProcessor.h"

#include <algorithm>
#include <utility>

#include "core/app/AppPaths.h"
#include "gui/host/HostScheduler.h"
#include "gui/host/UserExtensions.h"
#include "gui/plugins/PluginHostingSetup.h"
#include "plugin/StarterPatch.h"

namespace hum {

namespace {

constexpr juce::uint32 kSavedThrottleMs = 500, kSavedRefreshMs = 5000;
static_assert(PluginSession::kMacros == DawBridge::kControls, "the session keeps one mapping per DAW control");

}

HumusProcessor::HumusProcessor(Setup setup)
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    if (setup.loadUserExtensions) {
        static const bool extensionsLoaded = [] {
            loadUserExtensions();
            setUpPluginHosting(installedHumusApp());
            return true;
        }();
        juce::ignoreUnused(extensionsLoaded);
    }
    for (int i = 0; i < DawBridge::kControls; ++i)
        addParameter(macros_[(size_t) i] = new juce::AudioParameterFloat(
                         juce::ParameterID("macro" + juce::String(i + 1), 1), "Macro " + juce::String(i + 1),
                         juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    auto io = std::make_unique<PluginAudioIo>();
    audio_ = io.get();
    host_ = std::make_unique<EngineHost>(setup.scheduler != nullptr ? *setup.scheduler : messageThreadScheduler(),
                                         std::move(io), pluginEnvironment(installedHumusApp()));
    videoWindows_ = std::make_unique<VideoWindows>(*host_);
    host_->openVisuals = [this](const std::string& name, int width, int height) {
        videoWindows_->open(name, width, height);
    };
    host_->daw()->onControlMapped = [this](int slot, float value) {
        if (auto* m = macros_[(size_t) slot]) m->setValueNotifyingHost(value);
    };
    std::string error;
    loadPatchText(starterPatchText(), {}, error);
    refreshSaved(true);
    startTimerHz(30);
}

HumusProcessor::~HumusProcessor() {
    *alive_ = false;
    stopTimer();
    host_->openVisuals = nullptr;
    videoWindows_.reset();
    host_->stopAudio();
    host_.reset();
}

void HumusProcessor::timerCallback() {
    host_->serviceTick();
    refreshSaved(false);
}

bool HumusProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto out = layouts.getMainOutputChannelSet();
    const auto in = layouts.getMainInputChannelSet();
    const auto mono = juce::AudioChannelSet::mono(), stereo = juce::AudioChannelSet::stereo();
    if (out != mono && out != stereo) return false;
    return in.isDisabled() || in == mono || in == stereo;
}

void HumusProcessor::startPlaying() {
    host_->ensureAudio();
    if (!playChoicePending_) return;
    playChoicePending_ = false;
    if (playWhenStopped_ && !host_->isPlaying()) host_->play();
    if (!playWhenStopped_ && host_->isPlaying()) host_->stop();
}

void HumusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    inScratch_.setSize(2, samplesPerBlock);
    dawIn_.ensureSize(4096);
    audio_->prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    startPlaying();
}

void HumusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    DawBridge::Clock clock;
    if (auto* playHead = getPlayHead())
        if (const auto pos = playHead->getPosition()) {
            clock.known = true;
            clock.playing = pos->getIsPlaying();
            if (const auto bpm = pos->getBpm()) clock.bpm = *bpm;
            if (const auto ppq = pos->getPpqPosition()) clock.ppq = *ppq;
        }
    auto& daw = *host_->daw();
    daw.setClock(clock);
    for (int i = 0; i < DawBridge::kControls; ++i) daw.setControlValue(i, macros_[(size_t) i]->get());
    dawIn_.clear();
    dawIn_.addEvents(midi, 0, n, 0);
    midi.clear();
    daw.setMidi(&dawIn_, &midi);
    host_->setRenderOffline(isNonRealtime());

    const int ins = std::min(getTotalNumInputChannels(), buffer.getNumChannels());
    if (n > inScratch_.getNumSamples()) inScratch_.setSize(2, n, false, false, true);
    for (int c = 0; c < 2; ++c) {
        if (ins > 0) inScratch_.copyFrom(c, 0, buffer, std::min(c, ins - 1), 0, n);
        else inScratch_.clear(c, 0, n);
    }
    const float* in[2] = {inScratch_.getReadPointer(0), inScratch_.getReadPointer(1)};
    const int outs = std::min(2, buffer.getNumChannels());
    float* out[2] = {buffer.getWritePointer(0), buffer.getWritePointer(outs > 1 ? 1 : 0)};
    audio_->render(in, 2, out, outs, n);
    daw.setMidi(nullptr, nullptr);
    for (int c = outs; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, n);
}

bool HumusProcessor::loadPatch(const juce::File& file, std::string& error) {
    if (!host_->loadFile(pathOf(file), error)) return false;
    startPlaying();
    videoWindows_->reopenOutputs();
    return true;
}

bool HumusProcessor::loadPatchText(const std::string& text, const std::string& documentPath, std::string& error) {
    if (!host_->loadDocumentText(text, documentPath, error)) return false;
    startPlaying();
    return true;
}

std::string HumusProcessor::patchText() { return host_->documentText(); }

PluginSession HumusProcessor::currentSession() {
    PluginSession s;
    s.documentPath = host_->documentPath();
    for (int i = 0; i < PluginSession::kMacros; ++i) s.macros[(size_t) i] = host_->dawControlTargets(i);
    s.playWhenStopped = host_->isPlaying();
    s.text = host_->documentText();
    return s;
}

void HumusProcessor::refreshSaved(bool force) {
    const auto stamp = host_->changeStamp();
    const auto now = juce::Time::getMillisecondCounter();
    const auto age = now - savedAt_;
    if (!force && !(stamp != savedStamp_ && age >= kSavedThrottleMs) && age < kSavedRefreshMs) return;
    auto block = encodeSession(currentSession());
    const std::lock_guard<std::mutex> hold(savedLock_);
    saved_ = std::move(block);
    savedStamp_ = stamp;
    savedAt_ = now;
}

void HumusProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (juce::MessageManager::existsAndIsCurrentThread()) refreshSaved(true);
    const std::lock_guard<std::mutex> hold(savedLock_);
    dest.append(saved_.getData(), saved_.getSize());
}

void HumusProcessor::setStateInformation(const void* data, int size) {
    auto session = decodeSession(data, size);
    if (!session) return;
    if (juce::MessageManager::existsAndIsCurrentThread()) {
        applySession(std::move(*session));
        return;
    }
    juce::MessageManager::callAsync([this, alive = alive_, s = std::move(*session)]() mutable {
        if (*alive) applySession(std::move(s));
    });
}

void HumusProcessor::applySession(PluginSession session) {
    playWhenStopped_ = session.playWhenStopped;
    playChoicePending_ = true;
    std::string error;
    if (!session.text.empty() && loadPatchText(session.text, session.documentPath, error))
        videoWindows_->reopenOutputs();
    for (int i = 0; i < PluginSession::kMacros; ++i)
        host_->setDawControlTargets(i, session.macros[(size_t) i]);
    refreshSaved(true);
}

}
