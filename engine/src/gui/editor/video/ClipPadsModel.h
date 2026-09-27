// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <string>
#include <utility>

#include "gui/editor/LiveControls.h"
#include "gui/editor/files/FilePick.h"
#include "gui/editor/video/VideoModels.h"
#include "gui/host/BrickHost.h"
#include "hum/caps/Video.h"

namespace hum::video {

struct PadParams {
    std::string file, in, out, loop, launch;
};

struct PadRange {
    std::string file;
    double inSeconds = 0.0, outSeconds = 0.0;
    bool looped = false;
};

class ClipPadsModel {
public:
    static constexpr int kPads = VideoPadSource::kMaxClips;
    static constexpr int kParkTries = 200;

    ClipPadsModel(BrickHost& host, std::string organism, PadParams params)
        : host_(host), organism_(std::move(organism)), p_(std::move(params)) {}

    static std::string suffix(int i) { return std::to_string(i + 1); }
    static double centi(double seconds) { return std::round(seconds * 100.0) / 100.0; }

    std::string file(int i) const { return host_.liveParamText(organism_, p_.file + suffix(i)); }
    double in(int i) const { return host_.liveParamValue(organism_, p_.in + suffix(i)); }
    double out(int i) const { return host_.liveParamValue(organism_, p_.out + suffix(i)); }
    bool looped(int i) const { return host_.liveParamValue(organism_, p_.loop + suffix(i)) >= 0.5; }
    const std::string& launchParam() const { return p_.launch; }

    VideoPadSource* pads() const { return live::source<VideoPadSource>(host_, organism_); }

    void launch(int i);
    bool releaseLaunch(int i);
    void playPause(int i, PlaybackTarget* target);

    void toggleLoop(int i) { host_.setParam(organism_, p_.loop + suffix(i), looped(i) ? 0.0 : 1.0); }
    void setIn(int i, double seconds) { host_.setParam(organism_, p_.in + suffix(i), centi(seconds)); }
    void setOut(int i, double seconds) { host_.setParam(organism_, p_.out + suffix(i), centi(seconds)); }

    void stampIn(int i, PlaybackTarget* target) {
        if (target != nullptr) setIn(i, target->positionSeconds());
    }
    void stampOut(int i, PlaybackTarget* target) {
        if (target != nullptr) setOut(i, target->positionSeconds());
    }

    void load(int i, const std::string& path) { host_.setParamText(organism_, p_.file + suffix(i), path); }

    void clear(int i);
    void copy(int a, int b, bool move);
    void take(int i, const PadRange& r);
    files::FilePick request(int i, const std::string& videosDir) const;

    void forgetPark(int i) { parkWant_[(size_t) i] = -1.0; }

    void armPark(int i, double seconds);

    bool parkMoved(int i, double seconds) const { return std::abs(parkIn_[(size_t) i] - seconds) > 1.0e-6; }

    enum class Park { Idle, Parked, Chase };

    Park parkStep(int i, bool hasFrame, double framePts);

    double parkWant(int i) const { return parkWant_[(size_t) i]; }

private:
    BrickHost& host_;
    std::string organism_;
    PadParams p_;
    std::array<bool, kPads> release_{};
    std::array<double, kPads> parkWant_{};
    std::array<double, kPads> parkIn_{};
    std::array<int, kPads> parkTries_{};
};

}
