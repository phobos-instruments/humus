// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <string>

#include <juce_audio_processors/juce_audio_processors.h>

#include "gui/host/EngineHost.h"
#include "gui/host/HostScheduler.h"
#include "plugin/PluginAudioIo.h"
#include "plugin/PatchPanes.h"
#include "plugin/PluginSession.h"
#include "plugin/VideoWindows.h"

namespace hum {

struct PluginModeFirst {
    PluginModeFirst() {
        EngineHost::ignoreSavedDeviceFormat();
    }
};

class HumusProcessor : public juce::AudioProcessor, private juce::Timer {
public:
    struct Setup {
        HostScheduler* scheduler = nullptr;
        bool loadUserExtensions = true;
    };
    HumusProcessor() : HumusProcessor(Setup{}) {}
    explicit HumusProcessor(Setup setup);
    ~HumusProcessor() override;

    EngineHost& host() { return *host_; }
    VideoWindows& videoWindows() { return *videoWindows_; }
    PaneLayout& paneLayout() { return paneLayout_; }
    juce::AudioParameterFloat& macro(int i) { return *macros_[(size_t) i]; }
    bool loadPatch(const juce::File& file, std::string& error);
    bool loadPatchText(const std::string& text, const std::string& documentPath, std::string& error);
    std::string patchText();

    const juce::String getName() const override { return "Humus"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

private:
    void timerCallback() override;
    void startPlaying();
    PluginSession currentSession();
    void applySession(PluginSession session);
    void refreshSaved(bool force);

    PluginModeFirst modeFirst_;
    PluginAudioIo* audio_ = nullptr;
    std::array<juce::AudioParameterFloat*, DawBridge::kControls> macros_{};
    std::unique_ptr<EngineHost> host_;
    std::unique_ptr<VideoWindows> videoWindows_;
    PaneLayout paneLayout_;
    juce::AudioBuffer<float> inScratch_;
    juce::MidiBuffer dawIn_;
    std::mutex savedLock_;
    juce::MemoryBlock saved_;
    juce::uint64 savedStamp_ = 0;
    juce::uint32 savedAt_ = 0;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
    bool playWhenStopped_ = true;
    bool playChoicePending_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HumusProcessor)
};

}
