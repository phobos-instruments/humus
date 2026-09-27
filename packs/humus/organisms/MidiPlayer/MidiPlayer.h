// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/caps/Files.h"
#include "hum/caps/Layout.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"

#include "common/FmVoices.h"
#include "common/GmBank.h"
#include "common/MidiSong.h"
#include "common/Sf2Voices.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class MidiPlayer : public Organism, public FileTransportCap, public MidiNode,
                   public LayoutFacts {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }

    void deliverMidi(int, const MidiEvent*, int) override {}

    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        outCount_ = 0;
        return n;
    }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string& uri) override;
    void onTextChanged(const std::string& param, const std::string& text) override;

    std::string playSwitch() const override { return "Play"; }

    bool layoutFact(const std::string& name) const override {
        const int channel = channelOfFact(name);
        return channel >= 0 && channel < midisong::kChannels && used_[(size_t) channel];
    }

    static int channelOfFact(const std::string& name) {
        static constexpr char kPrefix[] = "channel";
        const auto n = sizeof(kPrefix) - 1;
        if (name.size() <= n || name.compare(0, n, kPrefix) != 0) return -1;
        int at = 0;
        for (auto i = n; i < name.size(); ++i) {
            if (name[i] < '0' || name[i] > '9') return -1;
            at = at * 10 + (name[i] - '0');
        }
        return at - 1;
    }
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
    void sendOut(int channel, int status, int a, int b);
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
    std::array<bool, midisong::kChannels> used_{};

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;

    juce::CriticalSection swapLock_;
    std::unique_ptr<midisong::Song> pending_;
    std::atomic<bool> hasPending_{false};
    std::atomic<std::int64_t> posSamples_{0};
    std::atomic<std::int64_t> lenSamples_{0};
    std::atomic<std::int64_t> seekReq_{-1};
};

}
