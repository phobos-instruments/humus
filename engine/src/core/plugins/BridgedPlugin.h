// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/plugins/BridgeClient.h"
#include "core/plugins/BridgeEditorControl.h"
#include "core/plugins/PluginNode.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class BridgedPlugin : public Organism, public MidiNode, public LatencyReporting,
                      public PluginNode, public BridgeEditorControl {
public:
    BridgedPlugin(std::string classRaw, const juce::PluginDescription& desc,
                  const juce::File& workerExe);
    ~BridgedPlugin() override;

    int numAudioInputs() const override { return ins_; }
    int numAudioOutputs() const override { return outs_; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override {}
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    std::string matchToken() const override { return classRaw_; }
    int latencySamples() const override { return pluginLatency_ + preparedBlock_; }
    bool takeLatencyChange() override { return latencyChanged_.exchange(false); }

    int numMidiInputs() const override { return acceptsMidi_ ? 1 : 0; }
    int numMidiOutputs() const override { return producesMidi_ ? 1 : 0; }
    void deliverMidi(int port, const MidiEvent* events, int count) override;
    int collectMidi(int port, MidiEvent* out, int capacity) override;

    const std::string& classRaw() const override { return classRaw_; }
    std::string getStateBase64() const override;
    void setStateBase64(const std::string& base64) override;
    void queueMidiMessage(const juce::MidiMessage& m) override;
    bool hasEditor() const override { return hasEditor_; }
    float paramValue(int index) const override;
    bool responding() const override {
        return !dead_ && responding_.load() && !client_->crashed();
    }

    void pollLifecycle();
    bool dead() const { return dead_; }
    void restart();

    void bridgeCreateEditor() override;
    void bridgeDestroyEditor() override;
    void bridgeSetFloating(bool floating) override;
    bool bridgeIsFloating() const override { return editorFloating_; }
    void bridgeSetEditorCallbacks(
        std::function<void(unsigned long xid, int w, int h)> onCreated,
        std::function<void(int w, int h)> onSize,
        std::function<void()> onFloatClosed) override;

    int bridgePid() const { return client_->childPid(); }
    bool launched() const { return launched_; }

private:
    void countMiss();
    void respawn();
    void wireEditor();
    void writeSubmitSlot(const float* const* in, int numIn, int numSamples,
                         const Transport& transport, BridgeClient* c,
                         BridgeShmHeader* h, uint32_t seq);

    std::string classRaw_;
    juce::File workerExe_;
    juce::String descXml_;
    int ins_ = 2, outs_ = 2;
    bool acceptsMidi_ = false, producesMidi_ = false, hasEditor_ = true;
    bool launched_ = false;
    bool dead_ = false;
    juce::uint32 lastRespawnMs_ = 0;
    int pluginLatency_ = 0;
    std::atomic<bool> latencyChanged_{false};
    int preparedBlock_ = 512;
    double preparedSampleRate_ = kDefaultSampleRate;

    std::unique_ptr<BridgeClient> client_;
    std::atomic<BridgeClient*> activeClient_{nullptr};
    std::unique_ptr<BridgeClient> retired_;

    bool editorOpen_ = false;
    bool editorFloating_ = false;
    std::function<void(unsigned long, int, int)> onEditorCreated_;
    std::function<void(int, int)> onEditorSize_;
    std::function<void()> onEditorFloatClosed_;

    BridgeClient* lastClientSeen_ = nullptr;
    uint32_t submittedSeq_ = 0;
    int misses_ = 0;
    int missLimit_ = 350;
    std::atomic<bool> responding_{true};
    std::vector<float> lastSentParams_;

    std::vector<MidiEvent> stagedIn_;
    int stagedInCount_ = 0;
    juce::CriticalSection midiLock_;
    std::vector<MidiEvent> pendingLive_;
    std::vector<MidiEvent> collected_;
    int collectedCount_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BridgedPlugin)
};

}
