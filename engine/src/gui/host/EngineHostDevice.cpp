// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/packs/Categories.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "hum/CameraCapture.h"
#include "hum/SerialPort.h"
#include "core/graph/GraphIo.h"
#include "core/net/LinkChase.h"
#include "core/packs/Roles.h"
#include "gui/app/AppSettings.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"
#include "hum/caps/Audio.h"
#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr const char* kAudioStateKey = "audio.state";
}

void EngineHost::requestFadeIn() {
    fadeTarget_.store(1.0f);
    fadeRestart_.store(true);
}

void EngineHost::persistAudioState() {
    if (auto state = devices_.createStateXml())
        AppSettings::instance().set(kAudioStateKey, state->toString());
}

void EngineHost::seedDeviceFormatFromSettings() {
    if (ignoreSavedFormat_) return;
    if (auto saved = juce::parseXML(AppSettings::instance().getString(kAudioStateKey))) {
        const double r = saved->getDoubleAttribute("audioDeviceRate", 0.0);
        const int b = saved->getIntAttribute("audioDeviceBufferSize", 0);
        if (r > 0.0) sampleRate_ = r;
        if (b > 0) block_ = b;
    }
}

void EngineHost::startAudioAsync(std::function<void(bool, std::string)> done) {
    if (audioRunning_ || audioStarting_) { if (done) done(true, {}); return; }
    audioStarting_ = true;
    fadeGainPub_.store(0.0f);
    fadeTarget_.store(0.0f);

    auto saved = std::shared_ptr<juce::XmlElement>(
        juce::parseXML(AppSettings::instance().getString(kAudioStateKey)).release());
    if (saved) saved->removeAttribute("audioDeviceRate");

    juce::MessageManager::callAsync([this, alive = hostAlive_, saved, done = std::move(done)] {
        if (!*alive) return;
        juce::String e = devices_.initialise(2, 2, saved.get(), true);
        if (e.isNotEmpty()) e = devices_.initialise(0, 2, saved.get(), true);
        if (e.isNotEmpty()) e = devices_.initialise(0, 2, nullptr, true);
        audioStarting_ = false;
        if (cancelStart_) {
            cancelStart_ = false;
            devices_.closeAudioDevice();
            if (done) done(false, "cancelled");
            return;
        }
        if (e.isNotEmpty()) { if (done) done(false, e.toStdString()); return; }
        devices_.addAudioCallback(this);
        audioRunning_ = true;
        requestFadeIn();
        if (done) done(true, {});
    });
}

bool EngineHost::ensureAudio() {
    if (audioRunning_ || audioStarting_) return true;
    startAudioAsync();
    return true;
}

void EngineHost::stopAudio() {
    if (audioStarting_) cancelStart_ = true;
    if (!audioRunning_) return;
    persistAudioState();
    fadeTarget_.store(0.0f);
    for (int i = 0; i < 60 && fadeGainPub_.load() > 0.0005f; ++i)
        juce::Thread::sleep(1);
    devices_.removeAudioCallback(this);
    devices_.closeAudioDevice();
    audioRunning_ = false;
    fadeGainPub_.store(0.0f);
    level_[0].store(0.0f);
    level_[1].store(0.0f);
}

void EngineHost::scheduleAuxChannelCheck() {
    if (auxCheckPending_) return;
    auxCheckPending_ = true;
    juce::MessageManager::callAsync([this, alive = hostAlive_] {
        if (!*alive) return;
        auxCheckPending_ = false;
        ensureAuxChannels();
    });
}

void EngineHost::ensureAuxChannels() {
    if (!audioRunning_) return;
    auto* dev = devices_.getCurrentAudioDevice();
    if (dev == nullptr || graph_ == nullptr) return;

    juce::BigInteger needIn, needOut;
    for (int i = 0; i < graph_->nodeCount(); ++i) {
        auto* c = graph_->organism(i);
        if (auto* hin = dynamic_cast<HardwareIn*>(c)) {
            const int ch = hin->channel();
            const int lo = hin->channelCount() > 1 ? 2 : 0;
            for (int k = 0; k < hin->channelCount(); ++k)
                if (ch + k >= lo && ch + k < kMaxDeviceChannels) needIn.setBit(ch + k);
        }
        if (auto* hout = dynamic_cast<HardwareOut*>(c)) {
            const int ch = hout->channel();
            if (ch >= 0 && ch < kMaxDeviceChannels) needOut.setBit(ch);
        }
        if (auto* tap = dynamic_cast<MasterTap*>(c)) {
            const int first = tap->firstChannel();
            for (int k = 0; k < tap->channels(); ++k)
                if (first + k >= 2 && first + k < kMaxDeviceChannels) needOut.setBit(first + k);
        }
    }
    if (needIn.isZero() && needOut.isZero()) return;

    const juce::BigInteger curIn = dev->getActiveInputChannels();
    const juce::BigInteger curOut = dev->getActiveOutputChannels();
    juce::BigInteger wantIn = curIn, wantOut = curOut;
    const int devIns = dev->getInputChannelNames().size();
    const int devOuts = dev->getOutputChannelNames().size();
    for (int b = 0; b < devIns && b < kMaxDeviceChannels; ++b)
        if (needIn[b]) wantIn.setBit(b);
    for (int b = 0; b < devOuts && b < kMaxDeviceChannels; ++b)
        if (needOut[b]) wantOut.setBit(b);
    if (wantIn == curIn && wantOut == curOut) return;

    if (dev->getName() != auxTriedDevice_) {
        auxTriedDevice_ = dev->getName();
        auxTriedIn_.clear();
        auxTriedOut_.clear();
    }
    if (wantIn == auxTriedIn_ && wantOut == auxTriedOut_) return;
    auxTriedIn_ = wantIn;
    auxTriedOut_ = wantOut;

    auto setup = devices_.getAudioDeviceSetup();
    setup.inputChannels = wantIn;
    setup.outputChannels = wantOut;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = false;
    deviceRestarts_.fetch_add(1, std::memory_order_relaxed);
    devices_.setAudioDeviceSetup(setup, true);
    persistAudioState();
}

float EngineHost::outputLevel(int channel) const {
    return (channel >= 0 && channel < 2) ? level_[channel].load() : 0.0f;
}

void EngineHost::audioDeviceAboutToStart(juce::AudioIODevice* d) {
    const double rate = d->getCurrentSampleRate();
    const int ringLen = (int) (kRingSeconds * rate);
    const int nIn = d->getActiveInputChannels().countNumberOfSetBits();
    {
        const juce::ScopedLock sl(lock_);
        sampleRate_ = rate;
        block_ = d->getCurrentBufferSizeSamples();
        outputLatency_ = d->getOutputLatencyInSamples();
        if ((int) perfAudioL_.size() != ringLen) {
            perfAudioL_.assign((size_t) ringLen, 0.0f);
            perfAudioR_.assign((size_t) ringLen, 0.0f);
            perfAudioWrite_.store(0, std::memory_order_relaxed);
        }
        buildChannelMap(d->getActiveInputChannels(), inMap_, kMaxDeviceChannels);
        buildChannelMap(d->getActiveOutputChannels(), outMap_, kMaxDeviceChannels);
        physInCount_ = 0;
        for (int p = 0; p < kMaxDeviceChannels; ++p)
            if (inMap_[p] >= 0) physInCount_ = p + 1;
        dcBuf_.assign((size_t) nIn, std::vector<float>((size_t) juce::jmax(block_, 64)));
        for (auto& b : dcBlock_) b.prepare(sampleRate_);
        masterFallback_ = outMap_[0] < 0 && outMap_[1] < 0;
        monoFold_ = d->getActiveOutputChannels().countNumberOfSetBits() == 1;
    }
    requestRebuild();
    if (std::getenv("HUMUS_AUDIO_DEBUG"))
        std::fprintf(stderr,
                     "[audio] start rate=%.0f block=%d ins=%d outs=%d"
                     " (previous device refused %d blocks, largest %d)\n",
                     sampleRate_, block_, nIn,
                     d->getActiveOutputChannels().countNumberOfSetBits(),
                     refusedBlocks_.exchange(0, std::memory_order_relaxed),
                     refusedLargest_.exchange(0, std::memory_order_relaxed));
}

void EngineHost::holdAudio(bool held) {
    if (held == audioHeld_) return;
    audioHeld_ = held;
    if (!audioRunning_) return;
    if (held) devices_.removeAudioCallback(this);
    else devices_.addAudioCallback(this);
}

}
