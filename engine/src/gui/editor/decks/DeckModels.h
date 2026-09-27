// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "core/params/ValueText.h"
#include "gui/host/BrickHost.h"
#include "gui/host/DeckEdits.h"
#include "gui/host/ModelHost.h"
#include "hum/dsp/BeatGrid.h"

namespace hum::decks {

class DeckPad {
public:
    static constexpr int kHotCues = 8;
    static constexpr double kLoopBeats[] = {1.0, 4.0, 8.0};
    static constexpr double kRollBeats[] = {0.125, 0.25, 0.5, 1.0};

    DeckPad(ModelHost& host, DeckEdits& decks, std::string organism, DeckParams params)
        : host_(host), decks_(decks), organism_(std::move(organism)), p_(std::move(params)) {}

    void hotCue(int index, bool clear);

    void fire(const std::string& trigger) {
        host_.setParam(organism_, trigger, 1.0);
        host_.setParam(organism_, trigger, 0.0);
    }
    void hold(const std::string& trigger, bool down) {
        host_.setParam(organism_, trigger, down ? 1.0 : 0.0);
    }

    bool hotCueLit(int index) const { return host_.liveParamValue(organism_, p_.hotCue(index)) >= 0.0; }
    bool looping() const { return host_.liveParamValue(organism_, p_.loop) >= 0.5; }

    void setCue() { decks_.setCue(organism_, p_); }
    void jumpCue() { decks_.jumpCue(organism_, p_); }
    void toggleLoop() { decks_.toggleLoop(organism_, p_); }
    void scaleLoop(double factor) { decks_.scaleBeatLoop(organism_, p_, factor); }
    void beatLoop(double beats) { decks_.setBeatLoop(organism_, p_, beats); }
    void beginRoll(double beats) { decks_.beginLoopRoll(organism_, p_, beats); }
    void endRoll() { decks_.endLoopRoll(organism_, p_); }

private:
    ModelHost& host_;
    DeckEdits& decks_;
    std::string organism_;
    DeckParams p_;
};

class PitchModel {
public:
    static constexpr double kBend = kBendPercent;

    PitchModel(ModelHost& host, DeckEdits& decks, std::string organism, std::string pitch,
               std::string range, std::string bpm)
        : host_(host), decks_(decks), organism_(std::move(organism)), pitch_(std::move(pitch)),
          range_(std::move(range)), bpm_(std::move(bpm)) {}

    double range() const { return std::max(1.0, host_.liveParamValue(organism_, range_)); }
    double pitch() const { return host_.liveParamValue(organism_, pitch_); }

    std::string pitchText() const { return (pitch() >= 0 ? "+" : "") + decimalText(pitch(), 1) + "%"; }
    std::string rangeText() const { return std::to_string((int) range()) + "%"; }

    double drivenPercent() const;
    std::string drivenText() const;

    void edit(double percent) { host_.editParam(organism_, pitch_, percent); }
    void beginDrag() { host_.beginParamDrag(organism_, pitch_); }
    void endDrag() { host_.endParamDrag(); }
    void bend(int direction, bool held) { decks_.setBend(organism_, held ? direction * kBend : 0.0); }

    void cycleRange();

private:
    ModelHost& host_;
    DeckEdits& decks_;
    std::string organism_, pitch_, range_, bpm_;
};

inline float peakOfWave(const std::vector<float>& peaks, double sample, std::int64_t length) {
    if (peaks.empty() || length <= 0 || sample < 0.0 || sample >= (double) length) return 0.0f;
    const auto at = (std::size_t) (sample / (double) length * (double) peaks.size());
    return peaks[at < peaks.size() ? at : peaks.size() - 1];
}

struct DeckViewParams {
    std::string bpm, cue, file, gridOffset, hotCuePrefix, key, loop, loopIn, loopOut, quantize,
        record;
};

class DeckViewModel {
public:
    static constexpr double kViewSeconds = 6.0;
    static constexpr int kZoomStops = 9;
    static constexpr int kZoomDefault = 4;

    static double zoomSeconds(int stop) {
        static constexpr double ladder[kZoomStops] = {0.5,  1.0,  2.0,  4.0, kViewSeconds,
                                                      12.0, 30.0, 60.0, 120.0};
        return ladder[stop < 0 ? 0 : (stop >= kZoomStops ? kZoomStops - 1 : stop)];
    }

    struct Window {
        std::int64_t length = 0, position = 0;
        double rate = 0.0, left = 0.0, span = 1.0;
        bool shows(double s) const { return s >= left && s <= left + span && s >= 0 && s <= (double) length; }
    };

    DeckViewModel(BrickHost& host, DeckEdits& decks, std::string organism, DeckViewParams params)
        : host_(host), decks_(decks), organism_(std::move(organism)), p_(std::move(params)) {}

    const DeckViewParams& params() const { return p_; }
    bool recording() const { return !p_.record.empty() && value(p_.record) >= 0.5; }
    double value(const std::string& param) const { return host_.liveParamValue(organism_, param); }

    Window window() const;

    std::vector<float> peaks() const { return decks_.waveform(organism_); }

    std::vector<float> peaksIn(double from, double to, int buckets) const {
        return decks_.waveBetween(organism_, (std::int64_t) from, (std::int64_t) to, buckets);
    }

    int zoom() const { return zoom_; }
    void zoomBy(int steps) {
        const int at = zoom_ + steps;
        zoom_ = at < 0 ? 0 : (at >= kZoomStops ? kZoomStops - 1 : at);
    }
    double viewSeconds() const { return zoomSeconds(zoom_); }
    std::string zoomText() const;

    BeatGrid grid() const {
        return BeatGrid{std::max(1.0, value(p_.bpm)), (std::int64_t) value(p_.gridOffset)};
    }

    std::vector<std::pair<double, bool>> beatLines(const Window& w) const;
    std::string bpmText() const;
    std::string infoText() const;
    static bool isAudioPath(const std::string& path);

    void load(const std::string& path) { host_.setParamText(organism_, p_.file, path); }

    std::int64_t sampleAt(int x, int x0, int width) const;
    void seekOverview(int x, int x0, int width);

    void gridAtPlayhead() { host_.setParam(organism_, p_.gridOffset, (double) decks_.position(organism_)); }
    void gridAt(std::int64_t sample) { host_.setParam(organism_, p_.gridOffset, (double) sample); }

    void scaleBpm(double k) { host_.setParam(organism_, p_.bpm, std::clamp(value(p_.bpm) * k, 20.0, 300.0)); }

    void beginScrub(int x);
    void scrubTo(int x, int width);

    void endScrub() { decks_.setScrub(organism_, false, 0.0); }

    void beginLoop(std::int64_t sample) { loopAnchor_ = sample; }
    void loopTo(std::int64_t sample);

private:
    BrickHost& host_;
    DeckEdits& decks_;
    std::string organism_;
    DeckViewParams p_;
    std::int64_t loopAnchor_ = 0, scrubFrom_ = 0;
    int zoom_ = kZoomDefault;
    int scrubX_ = 0;
};

}
