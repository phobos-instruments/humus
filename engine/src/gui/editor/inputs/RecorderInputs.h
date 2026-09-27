// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "gui/host/ModelHost.h"
#include "hum/dsp/LevelMeter.h"

namespace hum::input {

class LooperTracks {
public:
    LooperTracks(ModelHost& host, std::string organism, std::string prefix, int count)
        : host_(host), organism_(std::move(organism)), prefix_(std::move(prefix)), count_(count) {}

    int count() const { return count_; }
    std::string param(int track) const { return prefix_ + std::to_string(track + 1); }
    bool on(int track) const { return host_.liveParamValue(organism_, param(track)) >= 0.5; }
    void set(int track, bool enabled) { host_.setParam(organism_, param(track), enabled ? 1.0 : 0.0); }

    bool follow(int recording, int armed) {
        if (recording == recording_ && armed == armed_) return false;
        recording_ = recording;
        armed_ = armed;
        return true;
    }

    bool recording(int track) const { return track == recording_; }
    bool armed(int track) const { return track == armed_; }

private:
    ModelHost& host_;
    std::string organism_, prefix_;
    int count_;
    int recording_ = -2, armed_ = -2;
};

class TakeLane {
public:
    static constexpr int kHistory = 96;
    static constexpr double kFloorDb = -60.0;

    TakeLane(ModelHost& host, std::string organism, int track, std::string channelsPrefix)
        : host_(host), organism_(std::move(organism)), track_(std::max(1, track)),
          channelsPrefix_(std::move(channelsPrefix)), history_(kHistory, 0.0f) {}

    void poll(bool rolling, double nowMs) {
        const bool wasRolling = rolling_;
        rolling_ = rolling;
        if (rolling_ && !wasRolling) {
            std::fill(history_.begin(), history_.end(), 0.0f);
            startedMs_ = nowMs;
        }
        if (rolling_) seconds_ = (nowMs - startedMs_) / 1000.0;

        float lv[LevelMeter::kMax];
        const int have = host_.nodeMeter(organism_, lv, LevelMeter::kMax);
        const int from = firstChannel();
        const int span = std::max(1, channels());
        float loudest = 0.0f;
        for (int c = from; c < from + span && c < have; ++c) loudest = std::max(loudest, lv[c]);
        level_ = loudest;
        if (rolling_) {
            history_.erase(history_.begin());
            history_.push_back(loudest);
        }
    }

    static float bar(float linear) {
        if (linear <= 0.0f) return 0.0f;
        const double db = 20.0 * std::log10((double) linear);
        const double v = 1.0 - db / kFloorDb;
        return (float) (v < 0.0 ? 0.0 : (1.0 < v ? 1.0 : v));
    }

    std::string clockText() const {
        const int whole = (int) seconds_;
        const int secs = whole % 60;
        return std::to_string(whole / 60) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
    }

    int track() const { return track_; }
    bool rolling() const { return rolling_; }
    double seconds() const { return seconds_; }
    float level() const { return level_; }
    const std::vector<float>& history() const { return history_; }

    int channels() const {
        return (int) std::lround(host_.liveParamValue(organism_, channelsPrefix_ + std::to_string(track_)));
    }

    int firstChannel() const {
        int at = 0;
        for (int t = 1; t < track_; ++t)
            at += (int) std::lround(host_.liveParamValue(organism_, channelsPrefix_ + std::to_string(t)));
        return at;
    }

private:
    ModelHost& host_;
    std::string organism_;
    int track_;
    std::string channelsPrefix_;
    std::vector<float> history_;
    float level_ = 0.0f;
    double seconds_ = 0.0, startedMs_ = 0.0;
    bool rolling_ = false;
};

}
