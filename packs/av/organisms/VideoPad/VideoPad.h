// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <string>

#include <memory>
#include <mutex>
#include <vector>

#include "hum/caps/Files.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Params.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"

namespace hum {

class VideoPad : public Organism, public VideoNode, public MidiNode, public VideoPadSource,
                 public LiveParamRange, public RollListener, public FileLoader,
                 public ReloadOnParam, public ClipPlayhead {
public:
    static constexpr int kBaseNote = 60;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }

    void loadFromFile(const std::string& changed) override;
    bool reloadsOn(const std::string& param) const override {
        return param.rfind("File", 0) == 0;
    }
    double clipSeconds(int slot) const override {
        if (slot < 0 || slot >= kMaxClips) return -1.0;
        const double rate = srcRate_[(size_t) slot].load(std::memory_order_relaxed);
        if (rate <= 0.0) return -1.0;
        return heard_[(size_t) slot].load(std::memory_order_relaxed) / rate;
    }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override;
    void process(const float* const*, int, float* const*, int, int numSamples,
                 const Transport&) override;

    int numVideoInputs() const override { return 0; }
    int numVideoOutputs() const override { return separateOutlets() ? 1 + padCount() : 1; }
    unsigned videoLaunchCount() const override {
        return launches_.load(std::memory_order_relaxed);
    }

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }
    void deliverMidi(int, const MidiEvent* events, int count) override;
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    int clipCount() const override { return padCount(); }
    ClipState clipState() const override {
        ClipState s;
        s.active = active_.load(std::memory_order_relaxed);
        s.outgoing = outgoing_.load(std::memory_order_relaxed);
        s.phase = phase_.load(std::memory_order_relaxed);
        s.launches = launches_.load(std::memory_order_relaxed);
        return s;
    }
    void noteClipLength(int slot, double seconds) override {
        if (slot >= 0 && slot < kMaxClips)
            lengths_[(size_t) slot].store(seconds, std::memory_order_relaxed);
    }

    bool liveParamRange(const std::string& param, double& lo, double& hi) const override;
    bool rangeFollowsFile(const std::string& param, const std::string& fileParam) const override;
    void rolled() override;

    int padCount() const { return params.get("Pads", 1.0) >= 0.5 ? kMaxClips : kMaxClips / 2; }
    bool separateOutlets() const { return params.get("Outlets", 0.0) >= 0.5; }
    bool loaded(int slot) const;

    void launch(int slot);
    void stop();

private:
    bool primed_ = false;
    bool lastLaunch_[kMaxClips] = {};
    bool lastStop_ = false;
    int pendingMidi_[kMaxClips] = {};
    int pendingMidiCount_ = 0;
    std::atomic<int> pendingLaunch_{-1};
    std::atomic<int> active_{-1};
    std::atomic<int> outgoing_{-1};
    std::atomic<float> phase_{1.0f};
    std::atomic<unsigned> launches_{0};
    std::array<std::atomic<double>, kMaxClips> lengths_{};

    using Clip = SoundBytes;
    void readClip(int slot, float* const* out, int numSamples, float gain);

    mutable std::mutex clipLock_;
    std::array<std::shared_ptr<const Clip>, kMaxClips> clips_{};
    std::array<std::string, kMaxClips> uri_{};
    std::array<double, kMaxClips> pos_{};
    std::array<std::atomic<double>, kMaxClips> heard_{};
    std::array<std::atomic<double>, kMaxClips> srcRate_{};
    double sampleRate_ = kDefaultSampleRate;
};

}
