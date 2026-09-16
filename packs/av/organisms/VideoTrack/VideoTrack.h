// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "hum/caps/Files.h"
#include "hum/caps/Video.h"
#include "hum/Organism.h"
#include "hum/Pattern.h"

namespace hum {

struct VideoClipPlay {
    int id = 0;
    double startBeat = 0.0, lenBeats = 0.0;
    double clipStartBeat = 0.0, periodBeats = 0.0;
    bool loop = false;
    double offsetSeconds = 0.0;
    double gain = 1.0;
    double sourceBpm = 0.0;
    int warpMode = 0;
    double fadeInBeats = 0.0, fadeOutBeats = 0.0;
    float fadeInCurve = 0.0f, fadeOutCurve = 0.0f;
    bool reverse = false;
    std::string file;
};

std::vector<VideoClipPlay> videoClipPlays(const Pattern& pattern, double sampleRate);

VideoTimelineSource::Cue videoCueAt(const std::vector<VideoClipPlay>& clips, double beat,
                                    double tempo, bool rolling);

VideoTimelineSource::Cue videoUpcomingAt(const std::vector<VideoClipPlay>& clips, double beat,
                                         double tempo, double windowSeconds);

class VideoTrack : public Organism, public ClipArrangement, public VideoNode,
                   public VideoTimelineSource {
public:
    static constexpr double kPrerollSeconds = 1.5;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        setPattern(pattern_);
    }
    void reset() override {}
    void loadFrom(const OrganismState& state) override;
    void setPattern(const Pattern& pattern) override;
    void process(const float* const*, int, float* const*, int, int,
                 const Transport& transport) override;

    int numVideoInputs() const override { return 1; }
    int numVideoOutputs() const override { return 1; }

    Cue cue() const override;
    Cue upcoming() const override;
    std::string cueFile(int clip) const override;
    Cue cueAt(double beat, double tempo) const override;
    Cue upcomingAt(double beat, double tempo) const override;

private:
    void publish(std::atomic<int>& clip, std::atomic<double>& seconds, std::atomic<double>& rate,
                 std::atomic<float>& level, const Cue& c);

    double sampleRate_ = 48000.0;
    Pattern pattern_;
    std::vector<VideoClipPlay> clips_;
    std::vector<VideoClipPlay> pending_;
    std::vector<VideoClipPlay> table_;
    juce::CriticalSection loadLock_;
    std::atomic<bool> hasPending_{false};
    std::atomic<int> cueClip_{-1};
    std::atomic<double> cueSeconds_{0.0};
    std::atomic<double> cueRate_{1.0};
    std::atomic<float> cueLevel_{1.0f};
    std::atomic<bool> rolling_{false};
    std::atomic<int> nextClip_{-1};
    std::atomic<double> nextSeconds_{0.0};
    std::atomic<double> nextRate_{1.0};
    std::atomic<float> nextLevel_{1.0f};
};

}
