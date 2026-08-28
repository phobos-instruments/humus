#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/PluginNode.h"
#include "hum/Capabilities.h"
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
    std::atomic<bool> pushedOnce_{false};
    TransportPlayHead playHead_;
    juce::AudioBuffer<float> scratch_;
    juce::MidiBuffer midi_;
    juce::CriticalSection midiLock_;
    juce::MidiBuffer pendingMidi_;
    std::vector<MidiEvent> staged_;
    int stagedCount_ = 0;
    std::vector<float> lastSentParams_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostedPlugin)
};

}
