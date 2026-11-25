// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/caps/Audio.h"
#include "hum/caps/Files.h"
#include "hum/Organism.h"
#include "hum/Pattern.h"
#include "hum/dsp/ClipPlayer.h"
#include "hum/dsp/LevelMeter.h"
#include "hum/dsp/LiveWavWriter.h"

namespace hum {

class AudioTrack : public Organism, public ClipArrangement, public ClipRecorder,
                   public LevelMeterSource, public AudioFileWatcher {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void loadFrom(const OrganismState& state) override;
    void setPattern(const Pattern& pattern) override {
        pattern_ = pattern;
        clipsDirty_ = true;
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    bool startTake(const std::string& wavPath, double sampleRate) override;
    void stopTake() override;
    bool takeActive() const override { return takeActive_.load(std::memory_order_relaxed); }
    double takeStartBeat() const override { return takeStartBeat_.load(std::memory_order_relaxed); }
    std::int64_t takeLengthSamples() const override { return takeLen_.load(std::memory_order_relaxed); }
    int takeLaps(double* beats, std::int64_t* samples, int maxLaps) const override;
    void ensureClipsLoaded() override;
    void audioFileChanged(const std::string& path) override {
        if (cache_.erase(path) > 0) clipsDirty_ = true;
    }

    int meterChannels() const override { return 2; }
    float meterLevel(int ch) const override { return meter_.level(ch); }

private:
    void applyPending();
    std::shared_ptr<const ClipPcm> fileFor(const std::string& path);

    Pattern pattern_;
    bool clipsDirty_ = false;
    std::map<std::string, std::shared_ptr<const ClipPcm>> cache_;
    std::string takePath_;

    juce::CriticalSection loadLock_;
    std::vector<ClipVoice> pendingClips_;
    std::atomic<bool> hasPending_{false};

    ClipPlayer player_;

    LiveWavWriter writer_;
    std::atomic<bool> takeActive_{false};
    std::atomic<double> takeStartBeat_{-1.0};
    std::atomic<std::int64_t> takeLen_{0};
    std::atomic<int> lapCount_{0};
    std::array<double, kMaxLaps> lapBeats_{};
    std::array<std::int64_t, kMaxLaps> lapSamples_{};
    double lastCaptureBeat_ = -1.0;
    LevelMeter meter_;
};

}
