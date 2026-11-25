// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/plugins/PluginNode.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class HostedPlugin : public Organism, public MidiNode, public LatencyReporting,
                     public PluginNode {
public:
    HostedPlugin(std::unique_ptr<juce::AudioPluginInstance> instance, std::string classRaw);
    ~HostedPlugin() override;

    inline static std::function<bool(const std::string& classRaw)> shouldLeakInstance;
    inline static std::function<std::shared_ptr<void>(const std::string& classRaw)> disposeGuard;

    int numAudioInputs() const override { return ins_; }
    int numAudioOutputs() const override { return outs_; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    std::string matchToken() const override { return classRaw_; }

    int latencySamples() const override { return latency_; }
    bool takeLatencyChange() override { return latencyChanged_.exchange(false); }
    bool paramsPushed() const { return pushedOnce_.load(std::memory_order_relaxed); }

    const std::string& classRaw() const override { return classRaw_; }
    juce::AudioPluginInstance* instance() { return instance_.get(); }

    bool hasEditor() const override { return instance_->hasEditor(); }
    float paramValue(int index) const override {
        const auto& ps = instance_->getParameters();
        return index >= 0 && index < ps.size() ? ps[index]->getValue() : 0.0f;
    }

    void queueMidiMessage(const juce::MidiMessage& m) override {
        const juce::ScopedLock sl(midiLock_);
        pendingMidi_.addEvent(m, 0);
    }

    int numMidiInputs() const override { return instance_->acceptsMidi() ? 1 : 0; }
    int numMidiOutputs() const override { return instance_->producesMidi() ? 1 : 0; }
    void deliverMidi(int port, const MidiEvent* events, int count) override;
    int collectMidi(int port, MidiEvent* out, int capacity) override;

    std::string getStateBase64() const override;
    void setStateBase64(const std::string& base64) override;
    void loadFrom(const OrganismState& state) override;
    bool layoutChanged() const override;
    void adoptLayout() override;
    int touchedParam(unsigned& stamp) const override {
        stamp = touchStamp_.load(std::memory_order_acquire);
        return touched_.load(std::memory_order_relaxed);
    }

private:
    struct TransportPlayHead : juce::AudioPlayHead {
        std::atomic<double> bpm{120.0}, ppq{0.0};
        std::atomic<bool> playing{false};
        juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override {
            juce::AudioPlayHead::PositionInfo pi;
            pi.setBpm(bpm.load());
            pi.setPpqPosition(ppq.load());
            pi.setIsPlaying(playing.load());
            return pi;
        }
    };

    std::unique_ptr<juce::AudioPluginInstance> instance_;
    std::string classRaw_;
    int ins_ = 2, outs_ = 2;
    int latency_ = 0;
    std::atomic<bool> latencyChanged_{false};
    std::atomic<bool> pushedOnce_{false};
    TransportPlayHead playHead_;
    juce::AudioBuffer<float> scratch_;
    juce::MidiBuffer midi_;
    juce::CriticalSection midiLock_;
    juce::MidiBuffer pendingMidi_;
    std::vector<MidiEvent> staged_;
    int stagedCount_ = 0;
    void adoptInstanceValues();

    struct TouchListener : juce::AudioProcessorParameter::Listener {
        explicit TouchListener(HostedPlugin& owner) : owner_(owner) {}
        void parameterValueChanged(int index, float) override {
            if (juce::MessageManager::existsAndIsCurrentThread()) owner_.noteTouch(index);
        }
        void parameterGestureChanged(int index, bool starting) override {
            if (starting) owner_.noteTouch(index);
        }
        HostedPlugin& owner_;
    };
    void noteTouch(int index) {
        touched_.store(index, std::memory_order_relaxed);
        touchStamp_.fetch_add(1, std::memory_order_release);
    }
    void listenForTouches(bool on);
    std::atomic<int> touched_{-1};
    std::atomic<unsigned> touchStamp_{0};
    TouchListener touchListener_{*this};
    std::vector<float> lastSentParams_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostedPlugin)
};

}
