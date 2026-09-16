// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/caps/Audio.h"
#include "hum/caps/Files.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Params.h"
#include "hum/Organism.h"
#include "hum/PitchBend.h"
#include "hum/dsp/LevelMeter.h"
#include "hum/dsp/SoundFileBuffer.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Sampler : public Organism, public MidiNode, public FileLoader, public LiveMidiIn,
                public PatchBank, public LiveParamRange, public VoiceSource,
                public Recorder, public TextVoiceSource, public ReloadOnParam,
                public LevelMeterSource {
public:
    ~Sampler() override {
        delete pending_.exchange(nullptr);
        delete retired_.exchange(nullptr);
    }

    static constexpr int kSlots = 8;
    static constexpr int kMaxVoices = 16;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string& changedUri) override;

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }
    void deliverMidi(int port, const MidiEvent* events, int count) override;
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    void pushLiveMidi(const MidiEvent& e) override;

    int patchCount() const override { return (int) bankCache_.presetNames.size(); }
    std::string patchNameAt(int index) const override {
        return index >= 0 && index < (int) bankCache_.presetNames.size()
                   ? bankCache_.presetNames[(size_t) index] : std::string();
    }

    int meterChannels() const override { return 2; }
    float meterLevel(int channel) const override { return inMeter_.level(channel); }

    bool startRecording(const std::vector<RecordTarget>& targets, int punchMode,
                        double durationSeconds, double sampleRate, bool append) override;
    void stopRecording() override;
    bool isRecording() const override { return capturing_.load(); }
    bool consumeAutoStop() override { return autoStop_.exchange(false); }
    int channels() const override { return 2; }

    bool reloadsOn(const std::string& param) const override {
        return param == "Bits" || param == "Rate";
    }

    bool takeVoiceTexts(std::vector<std::pair<std::string, std::string>>& out) override;
    bool voiceParams(std::vector<std::pair<std::string, double>>& out) const override;

    bool liveParamRange(const std::string& param, double& lo, double& hi) const override {
        if (param != "Preset" || bankCache_.presetNames.empty()) return false;
        lo = 1.0;
        hi = (double) bankCache_.presetNames.size();
        return true;
    }

private:
    struct SampleData {
        juce::AudioBuffer<float> buf;
        double srcRate = kDefaultSampleRate;
        std::string uri;
    };

    struct Zone {
        int sample = -1;
        int slot = -1, preset = -1;
        int rootKey = 60;
        int keyLo = 0, keyHi = kMidiMax, velLo = 0, velHi = kMidiMax;
        double tuneCents = 0.0;
        int loopStart = 0, loopEnd = 0;
        int loopMode = -1;
        float gain = 1.0f, panL = 1.0f, panR = 1.0f;
        double attackMs = 2.0, decayMs = 120.0, releaseMs = 150.0;
        float sustain = 1.0f;
    };

    struct Kit {
        std::vector<SampleData> samples;
        std::vector<Zone> zones;
        std::vector<std::string> presetNames;
    };
    struct Voice {
        int zone = -1;
        int note = -1;
        double pos = 0.0, rate = 1.0;
        float gain = 0.0f, env = 0.0f;
        int stage = 0;
        std::uint32_t age = 0;
        float attackInc = 1.0f, decayCoef = 0.0f, releaseCoef = 0.0f, sustain = 1.0f;
        float panL = 1.0f, panR = 1.0f;
        int endIdx = 0, loopLen = 0;
    };

    void applyPending();
    void publish(Kit&& kit);
    int rootOf(const Zone& z) const;
    int selectedPreset() const;
    void buildBank(const std::string& uri, Kit& out);
    void startVoice(const Zone& zone, int zoneIndex, int note, float velocity);
    void refreshVoiceEnvelopes();
    void updateVoice(Voice& v);
    bool bankLoaded() const { return !kit_.presetNames.empty(); }
    void noteOn(int note, float velocity);
    void noteOff(int note);
    void allOff(bool hard);
    void handleEvent(const MidiEvent& e);
    void renderAdd(float* left, float* right, int numSamples);
    int pickZone(int note, bool kit) const;

    Kit kit_;
    PitchBend bend_;
    double bendRatio_ = 1.0;

    std::atomic<Kit*> pending_{nullptr};
    std::atomic<Kit*> retired_{nullptr};

    std::array<SampleData, kSlots> slotCache_;
    std::array<std::string, kSlots> slotUri_;
    std::array<SoundFileInfo, kSlots> slotInfo_;
    mutable std::array<int, kSlots> rootSeed_{};

    LevelMeter inMeter_;
    juce::AudioBuffer<float> capture_;
    std::atomic<bool> waitingForLevel_{false};
    std::atomic<bool> capturing_{false};
    std::atomic<bool> autoStop_{false};
    std::atomic<int> capturePos_{0};
    std::atomic<int> captureLimit_{0};
    std::string capturePath_;
    int captureSlot_ = 1;
    bool captureReady_ = false;

    int lastBits_ = 24, lastRate_ = 48;

    Kit bankCache_;
    std::string bankUri_;

    std::array<Voice, kMaxVoices> voices_;
    std::uint32_t voiceClock_ = 0;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::mutex liveLock_;
    std::array<MidiEvent, 128> liveQ_;
    int liveCount_ = 0;
};

}
