#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/AudioGraph.h"
#include "core/GraphIo.h"
#include "io/PatchDocument.h"

namespace hum {

class HumusProcessor : public juce::AudioProcessor {
public:
    HumusProcessor();

    bool loadPatchFile(const juce::File& file, std::string& error);
    bool loadPatchText(const std::string& amhXml, std::string& error);
    juce::String patchName() const { return patchName_; }
    float lastPeak(int channel) const { return peak_[channel & 1].load(); }

    void setLiveParam(const std::string& organism, const std::string& param, double v);
    double liveParam(const std::string& organism, const std::string& param,
                     double fallback) const;

    void setFreeRun(bool on) { freeRun_.store(on, std::memory_order_relaxed); }
    bool freeRun() const { return freeRun_.load(std::memory_order_relaxed); }
    void rewindTransport() { rewindReq_.store(true, std::memory_order_relaxed); }

    bool transportPlaying() const { return graph_ && graph_->transport().playing(); }
    double transportTempo() const {
        return graph_ ? graph_->transport().tempo() : model_.clock.tempo;
    }
    double transportBeats() const { return graph_ ? graph_->transport().beats() : 0.0; }

    static constexpr int kNumMacros = 8;
    const PatchDocumentModel& model() const { return model_; }
    void setMacroMapping(int i, const std::string& organism, const std::string& param);
    std::pair<std::string, std::string> macroMapping(int i) const;
    float macroValue(int i) const;
    void setMacroValue(int i, float v);

    const juce::String getName() const override { return "Humus"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

private:
    void applyPending();
    void driveTransportFromHost();
    void applyMacros();
    void applyParamEdits();

    struct MacroMap { std::string organism, param; float lo = 0.0f, hi = 1.0f; };
    MacroMap macros_[kNumMacros];
    juce::AudioParameterFloat* macroParam_[kNumMacros] {};
    juce::CriticalSection macroLock_;
    PatchDocumentModel model_;

    std::unique_ptr<AudioGraph> graph_;
    std::vector<MasterTap*> masters_;
    std::vector<HardwareOut*> auxes_;
    std::unique_ptr<AudioGraph> pending_;
    std::atomic<bool> hasPending_{false};
    juce::CriticalSection stageLock_;

    struct ParamEdit { std::string organism, param; double value; };
    std::vector<ParamEdit> pendingEdits_;
    juce::CriticalSection editLock_;

    std::string docText_;
    std::unique_ptr<juce::XmlElement> docXml_;
    juce::String patchName_;
    double sampleRate_ = 44100.0;
    int blockSize_ = 512;
    double lastPpq_ = -1.0;
    bool hostWasPlaying_ = false;
    std::atomic<bool> freeRun_{true};
    std::atomic<bool> rewindReq_{false};
    juce::AudioBuffer<float> inScratch_;
    std::atomic<float> peak_[2] {0.0f, 0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HumusProcessor)
};

}
