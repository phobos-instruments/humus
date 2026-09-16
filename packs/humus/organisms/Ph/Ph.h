// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <mutex>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/caps/Files.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Params.h"
#include "hum/Organism.h"
#include "hum/PitchBend.h"
#include "hum/ParamRef.h"

#include "common/RateRing.h"

#include "common/FmChip.h"
#include "common/FmMods.h"
#include "Ph/PhOpl.h"
#include "Ph/PhOpm.h"
#include "Ph/PhSixOp.h"
#include "common/FmVoice.h"

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
    void applyBend();
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        fileVoice_.store((int) voiceOf(params.getText("File")), std::memory_order_relaxed);
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        if (param == "File") fileVoice_.store((int) voiceOf(text), std::memory_order_relaxed);
    }

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
            && voiceOf(params.getText("File")) != Voice::SixOp) {
            lo = hi = 0.0;
            return true;
        }
        return false;
    }

    bool voiceParams(std::vector<std::pair<std::string, double>>& out) const override;

    int playingSlot() const {
        return voice_ == Voice::Chip  ? chip_.currentSlot()
             : voice_ == Voice::Opm   ? opm_.currentSlot()
             : voice_ == Voice::Opl   ? opl_.currentSlot()
                                      : six_.currentSlot();
    }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    enum class Voice { SixOp, Chip, Opm, Opl };
    static Voice voiceOf(const std::string& bankRef);

    static constexpr const char* kFactorySixOp = "factory:six-op";
    static constexpr const char* kFactoryChip = "factory:chip";
    static constexpr const char* kFactoryOpm = "factory:opm";
    static constexpr const char* kFactoryOpp = "factory:opp";
    static constexpr const char* kFactoryOpl = "factory:opl";

private:
    void pumpSixOp();
    void pumpChip();
    void pumpOpm();
    void pumpOpl();
    FmMods readMods() const;
    FmVoice readVoice(const FmVoice& fromBank) const;

    struct OpParams { ParamRef on, level, ratio, detune, attack, decay, sustain, release; };
    static std::array<OpParams, 6> makeOpParams();
    struct ModParams { ParamRef bright, attack, release, detune, vibrato, speed; };

    PhSixOp six_;
    FmChip chip_;
    PhOpm opm_;
    PhOpl opl_;
    Voice voice_ = Voice::SixOp;
    std::atomic<int> fileVoice_{0};
    int patchSlot_ = -1;
    FmMods mods_;
    FmVoice voice_params_;
    ParamRef algorithmRef_ {"Algorithm"}, feedbackRef_ {"Feedback"};
    std::array<OpParams, 6> opParams_ = makeOpParams();
    ModParams modParams_ {ParamRef("Bright"), ParamRef("Attack"), ParamRef("Release"),
                          ParamRef("Detune"), ParamRef("Vibrato"), ParamRef("Speed")};
    std::string loadedUri_;

    RateRing ring_;

    juce::CriticalSection ioLock_;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
    PitchBend bend_;
};

}
