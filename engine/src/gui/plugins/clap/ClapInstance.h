// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/plugins/clap/ClapEvents.h"
#include "gui/plugins/clap/ClapParameter.h"
#include "gui/plugins/clap/ClapSession.h"

namespace hum::claphost {

class Instance : public juce::AudioPluginInstance, private SessionOwner {
public:
    Instance(std::unique_ptr<Session> session, const juce::PluginDescription& description);
    ~Instance() override;

    const juce::String getName() const override { return description_.name; }
    void fillInPluginDescription(juce::PluginDescription& d) const override { d = description_; }

    void prepareToPlay(double sampleRate, int maxBlock) override;
    void releaseResources() override;
    void reset() override { resetPending_.store(true); }
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    using AudioPluginInstance::processBlock;
    void setNonRealtime(bool offline) noexcept override;

    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override { return ports_.noteIn; }
    bool producesMidi() const override { return ports_.noteOut; }
    bool hasEditor() const override { return session_->hasWindow(); }
    juce::AudioProcessorEditor* createEditor() override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

    Session& session() { return *session_; }
    std::function<bool(uint32_t, uint32_t)> onResizeRequest;

protected:
    bool canAddBus(bool) const override { return true; }
    bool canRemoveBus(bool) const override { return true; }
    bool canApplyBusCountChange(bool isInput, bool adding, BusProperties& props) override;

private:
    static BusesProperties busesFor(const PortLayout& ports);
    void buildParameters();
    void refreshParameterValues();
    void activate();
    void deactivate();
    void flushOnMessageThread();
    void scheduleFlush();
    void stopProcessingLocked();
    void allocateBuffers();
    void adoptPorts();
    void reshapeBuses(bool isInput, const std::vector<int>& channels);
    void pushParameterChanges(EventList& into);
    void readOutputEvents(juce::MidiBuffer& midi);
    const clap_event_transport_t* updateTransport();
    void wireBuffers(juce::AudioBuffer<float>& buffer, int numSamples);
    void pushMidi(EventList& into, const juce::MidiBuffer& midi);

    void restartRequested() override;
    void paramsChanged(uint32_t flags) override;
    void latencyChanged() override;
    void flushRequested() override;
    void portsChanged() override;
    bool resizeRequested(uint32_t w, uint32_t h) override;

    std::unique_ptr<Session> session_;
    juce::PluginDescription description_;
    PortLayout ports_;
    std::vector<ClapParameter*> params_;
    std::unordered_map<clap_id, ClapParameter*> byId_;
    std::atomic<bool> anyPending_{false};

    bool active_ = false;
    bool processing_ = false;
    double sampleRate_ = 48000.0;
    int maxBlock_ = 512;
    int64_t steadyTime_ = 0;
    std::atomic<bool> resetPending_{false};
    std::atomic<bool> flushPending_{false};
    std::atomic<bool> audioRunning_{false};

    EventList inEvents_, outEvents_;
    NoteTranslator notes_;
    juce::AudioBuffer<float> inputCopy_;
    std::vector<float*> inPtrs_, outPtrs_;
    std::vector<clap_audio_buffer_t> inBufs_, outBufs_;
    clap_event_transport_t transport_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Instance)
};

}
