// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace hum {

inline constexpr double kBendPercent = 4.0;

inline constexpr int kNudgeSteps = 5;

inline double nudgeStepSeconds(int step) {
    static constexpr double ladder[kNudgeSteps] = {0.001, 0.01, 0.1, 1.0, 5.0};
    return ladder[step < 0 ? 0 : (step >= kNudgeSteps ? kNudgeSteps - 1 : step)];
}

inline std::string nudgeStepText(int step) {
    const double seconds = nudgeStepSeconds(step);
    if (seconds < 1.0) return std::to_string((int) (seconds * 1000.0 + 0.5)) + "ms";
    return std::to_string((int) (seconds + 0.5)) + "s";
}

struct DeckParams {
    std::string bpm, cue, gridOffset, hotCuePrefix, loop, loopBeats, loopIn, loopOut, quantize;
    std::string hotCue(int index) const { return hotCuePrefix + std::to_string(index); }
};

class DeckEdits {
public:
    virtual ~DeckEdits() = default;

    virtual std::int64_t position(const std::string& name) = 0;
    virtual std::int64_t length(const std::string& name) = 0;
    virtual double       sampleRate(const std::string& name) = 0;
    virtual double       effectiveBpm(const std::string& name) = 0;
    virtual void         seek(const std::string& name, std::int64_t sample) = 0;
    virtual void         setBend(const std::string& name, double percent) = 0;
    virtual void         setScrub(const std::string& name, bool active, double targetSample) = 0;
    virtual std::vector<float> waveform(const std::string& name) = 0;
    virtual std::vector<float> waveBetween(const std::string& name, std::int64_t from,
                                           std::int64_t to, int buckets) = 0;

    virtual void setCue(const std::string& name, const DeckParams& p) = 0;
    virtual void jumpCue(const std::string& name, const DeckParams& p) = 0;
    virtual void setHotCue(const std::string& name, const DeckParams& p, int index) = 0;
    virtual void jumpHotCue(const std::string& name, const DeckParams& p, int index) = 0;
    virtual void clearHotCue(const std::string& name, const DeckParams& p, int index) = 0;
    virtual void setBeatLoop(const std::string& name, const DeckParams& p, double beats) = 0;
    virtual void scaleBeatLoop(const std::string& name, const DeckParams& p, double factor) = 0;
    virtual void toggleLoop(const std::string& name, const DeckParams& p) = 0;
    virtual void beginLoopRoll(const std::string& name, const DeckParams& p, double beats) = 0;
    virtual void endLoopRoll(const std::string& name, const DeckParams& p) = 0;
};

}
