#pragma once
#include <array>
#include <mutex>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

#include "Ph/PhChip.h"
#include "Ph/PhMods.h"
#include "Ph/PhSixOp.h"
#include "Ph/PhVoice.h"

namespace hum {

class Ph : public Organism, public MidiNode, public LiveMidiIn, public FileLoader,
           public PatchBank, public LiveParamRange, public VoiceSource {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    int liveMidiPort() const override { return -1; }
    void pushLiveMidi(const MidiEvent& e) override {
        std::lock_guard<std::mutex> g(liveLock_);
        if (liveCount_ < (int) liveQ_.size()) liveQ_[(size_t) liveCount_++] = e;
    }

    void loadFromFile(const std::string& uri) override;

    int patchCount() const override;
    std::string patchNameAt(int index) const override;

    bool liveParamRange(const std::string& param, double& lo, double& hi) const override {
        if (param == "Patch") {
            lo = 1.0;
            hi = (double) std::max(1, patchCount());
            return true;
        }
        if ((param == "Op5_On" || param == "Op6_On")
            && voiceOf(params.getText("File")) == Voice::Chip) {
            lo = hi = 0.0;
            return true;
        }
        return false;
    }

    bool voiceParams(std::vector<std::pair<std::string, double>>& out) const override;

    int playingSlot() const {
        return voice_ == Voice::Chip ? chip_.currentSlot() : six_.currentSlot();
    }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    enum class Voice { SixOp, Chip };
    static Voice voiceOf(const std::string& bankRef);

    static constexpr const char* kFactorySixOp = "factory:six-op";
    static constexpr const char* kFactoryChip = "factory:chip";

private:
    void pumpSixOp();
    void pumpChip();
    PhMods readMods() const;
    PhVoice readVoice(const PhVoice& fromBank) const;

    PhSixOp six_;
    PhChip chip_;
    Voice voice_ = Voice::SixOp;
    int patchSlot_ = -1;
    PhMods mods_;
    PhVoice voice_params_;
    std::string loadedUri_;

    static constexpr int kRing = 4096;
    std::array<float, kRing> ringL_{}, ringR_{};
    int ringWrite_ = 0;
    double ringRead_ = 0.0;

    juce::CriticalSection ioLock_;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
};

}
