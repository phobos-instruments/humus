// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/caps/Files.h"
#include "hum/Organism.h"

#include "common/FmVoices.h"
#include "common/GmBank.h"
#include "common/MidiSong.h"
#include "common/Sf2Voices.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class MidiPlayer : public Organism, public FileTransportCap {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string& uri) override;
    void onTextChanged(const std::string& param, const std::string& text) override;

    std::int64_t playbackPositionSamples() const override { return posSamples_.load(); }
    std::int64_t fileLengthSamples() const override { return lenSamples_.load(); }
    double playbackSampleRate() const override { return sampleRate_; }
    void requestSeekSamples(std::int64_t sample) override { seekReq_.store(sample); }

    enum class Engine { Fm, SoundFont };
    Engine engine() const { return engine_; }
    int soundingVoices() const {
        return engine_ == Engine::SoundFont ? sf2_.sounding() : voices_.sounding();
    }
    const midisong::Song& song() const { return song_; }

private:
    void adoptPending();
    void loadBank(const std::string& uri);
    void emitUpTo(double beat);
    void voiceNoteOn(int channel, int note, int velocity);
    void voiceNoteOff(int channel, int note);
    void voiceAllOff(int channel);
    void voiceProgram(int channel, int program);
    void voiceBend(int channel, int wheel);
    void rewind();
    int programFor(int channel) const;
    bool muted(int channel) const;

    midisong::Song song_;
    GmBank bank_;
    FmVoices voices_;
    Sf2Voices sf2_;
    Engine engine_ = Engine::Fm;

    double sampleRate_ = kDefaultSampleRate;
    double cursor_ = 0.0;
    size_t next_ = 0;
    bool wasPlaying_ = false;
    std::string loadedSong_, loadedBank_;

    juce::CriticalSection swapLock_;
    std::unique_ptr<midisong::Song> pending_;
    std::atomic<bool> hasPending_{false};
    std::atomic<std::int64_t> posSamples_{0};
    std::atomic<std::int64_t> lenSamples_{0};
    std::atomic<std::int64_t> seekReq_{-1};
};

}
