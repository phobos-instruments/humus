// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "core/params/ValueText.h"
#include "gui/host/FilePlayback.h"
#include "gui/host/ModelHost.h"

namespace hum::files {

inline constexpr double kNudgeSeconds = 10.0;
inline constexpr int kNudgeHoldTicks = 9;
inline constexpr double kNudgeHoldStart = 0.5;
inline constexpr double kNudgeHoldGrowth = 1.07;
inline constexpr double kNudgeHoldCeiling = 5.0;

inline double nudgeHoldStep(int ticksHeld) {
    if (ticksHeld < kNudgeHoldTicks) return 0.0;
    const double grown = kNudgeHoldStart
                       * std::pow(kNudgeHoldGrowth, (double) (ticksHeld - kNudgeHoldTicks));
    return grown < kNudgeHoldCeiling ? grown : kNudgeHoldCeiling;
}

inline std::int64_t nudgedSample(std::int64_t position, std::int64_t length, double sampleRate,
                                 double seconds) {
    if (length <= 0 || sampleRate <= 0.0) return -1;
    std::int64_t at = position + (std::int64_t) (seconds * sampleRate);
    if (at < 0) at = 0;
    if (at > length - 1) at = length - 1;
    return at;
}

class TransportModel {
public:
    TransportModel(ModelHost& host, FilePlayback& playback, std::string organism, std::string activeParam,
                   std::string loopParam, std::string recordParam = {})
        : host_(host), playback_(playback), organism_(std::move(organism)), active_(std::move(activeParam)),
          loop_(std::move(loopParam)), record_(std::move(recordParam)) {}

    bool active() const { return host_.liveParamValue(organism_, active_) >= 0.5; }
    bool looping() const { return host_.liveParamValue(organism_, loop_) >= 0.5; }
    bool arms() const { return !record_.empty(); }
    bool armed() const { return arms() && host_.liveParamValue(organism_, record_) >= 0.5; }

    void setActive(bool on) {
        if (on) playback_.ensureAudio();
        host_.setParam(organism_, active_, on ? 1.0 : 0.0);
    }

    void togglePlay() { setActive(!active()); }
    void rewind() { playback_.seek(organism_, 0); }
    void stop() {
        if (!record_.empty() && host_.liveParamValue(organism_, record_) >= 0.5)
            host_.setParam(organism_, record_, 0.0);
        setActive(false);
        playback_.seek(organism_, 0);
    }
    void toggleLoop() { host_.setParam(organism_, loop_, looping() ? 0.0 : 1.0); }

    void toggleArmed() {
        if (!arms()) return;
        const bool on = !armed();
        if (on) playback_.ensureAudio();
        host_.setParam(organism_, record_, on ? 1.0 : 0.0);
    }

    double positionSeconds() const {
        const double sr = playback_.playbackSampleRate(organism_);
        return sr > 0.0 ? (double) playback_.playbackPosition(organism_) / sr : 0.0;
    }

    double lengthSeconds() const {
        const double sr = playback_.playbackSampleRate(organism_);
        return sr > 0.0 ? (double) playback_.playbackLength(organism_) / sr : 0.0;
    }

    void nudgeSeconds(double seconds) {
        const auto at = nudgedSample(playback_.playbackPosition(organism_),
                                     playback_.playbackLength(organism_),
                                     playback_.playbackSampleRate(organism_), seconds);
        if (at >= 0) playback_.seek(organism_, at);
    }

    void seekTo(double fraction) {
        const std::int64_t len = playback_.playbackLength(organism_);
        if (len <= 0) return;
        playback_.seek(organism_, (std::int64_t) (fraction * (double) len));
    }

    double fraction() const {
        const std::int64_t len = playback_.playbackLength(organism_);
        if (len <= 0) return 0.0;
        const double f = (double) playback_.playbackPosition(organism_) / (double) len;
        return f < 0.0 ? 0.0 : (1.0 < f ? 1.0 : f);
    }

    std::string timeText() const {
        const double sr = playback_.playbackSampleRate(organism_);
        return clock(playback_.playbackPosition(organism_), sr) + " / " + clock(playback_.playbackLength(organism_), sr);
    }

    static std::string clock(std::int64_t samples, double sr) {
        const double secs = sr > 0.0 ? (double) samples / sr : 0.0;
        const int mins = (int) (secs / 60.0);
        const double rem = secs - mins * 60.0;
        auto tail = decimalText(rem, 3);
        if (tail.size() < 6) tail.insert(0, 6 - tail.size(), '0');
        return std::to_string(mins) + ":" + tail;
    }

private:
    ModelHost& host_;
    FilePlayback& playback_;
    std::string organism_, active_, loop_, record_;
};

}
