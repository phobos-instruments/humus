// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/PitchBend.h"
#include "hum/dsp/Biquad.h"
#include "hum/dsp/Oversampler.h"
#include "hum/dsp/Prepared.h"
#include "hum/dsp/WaveTable.h"

namespace hum {

class Wave : public Organism, public MidiNode, public LiveMidiIn {
public:
    static constexpr int kVoices = 8;
    static constexpr int kLevels = 10;
    static constexpr int kMaxFrames = 16;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override;
    void reset() override;
    void loadFrom(const OrganismState& state) override;
    void onTextChanged(const std::string& param, const std::string& text) override;

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

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    struct Voice {
        int note = -1;
        float vel = 1.0f;
        bool gate = false;
        double env = 0.0;
        double phase = 0.0;
        double uphase[6] = {};
        double subPhase = 0.0;
        double hz = 0.0;
    };

    struct Table {
        float frames[kMaxFrames][kWaveTableLen] = {};
        float mip[kMaxFrames][kLevels][kWaveTableLen] = {};
        float fft[kMaxFrames][2 * kWaveTableLen] = {};
        int frameCount = 1;
    };
    static std::unique_ptr<Table> buildTable(const std::string& text, bool presetWhenEmpty);
    void syncTable();

    void noteOn(int note, int vel);
    void noteOff(int note);
    int levelFor(double hz, double sr) const {
        const double maxPartial = 0.45 * sr / std::max(1.0, hz);
        int level = 0;
        while (level < kLevels - 1 && (double) (kWaveTableLen / 2 >> level) > maxPartial)
            ++level;
        return level;
    }

    std::unique_ptr<Table> table_;
    Prepared<std::unique_ptr<Table>> pendingTable_;
    std::string appliedText_;
    float morphMip_[kLevels][kWaveTableLen] = {};
    int morphF0_ = -1, morphF1_ = -1;
    float morphFr_ = -1.0f;
    void buildMorph(int f0, int f1, float fr);
    std::unique_ptr<juce::dsp::FFT> fft_;
    float lvl_[2 * kWaveTableLen] = {};

    std::array<Voice, kVoices> voices_;
    int next_ = 0;
    PitchBend bend_;
    double bendRatio_ = 1.0;
    double freePhase_ = 0.0;
    double freeSubPhase_ = 0.0;
    double freeEnv_ = 0.0;
    Biquad lp_, lpR_;
    Oversampler driveOsL_, driveOsR_;
    double lpHz_ = -1.0, lpQ_ = -1.0;
    std::vector<float> warpRamp_;
    double warpSm_ = 0.0;
    std::vector<float> posRamp_;
    double posSm_ = 0.0;
    double driveSm_ = 0.0;
    bool posPrimed_ = false;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveQ_;
    int liveCount_ = 0;
    std::mutex liveLock_;
};

}
