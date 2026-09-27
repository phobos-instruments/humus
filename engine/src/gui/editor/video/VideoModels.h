// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "core/params/ValueText.h"
#include "gui/editor/Words.h"
#include "gui/editor/LiveControls.h"
#include "gui/host/ModelHost.h"
#include "hum/caps/Video.h"

namespace hum::video {

class PlaybackTarget {
public:
    virtual ~PlaybackTarget() = default;
    virtual double lengthSeconds() = 0;
    virtual double positionSeconds() = 0;
    virtual bool isPaused() const = 0;
    virtual void setPaused(bool paused) = 0;
    virtual void seekSeconds(double seconds) = 0;
};

class VideoTransportModel {
public:
    static std::string clock(double seconds);

    static bool live(PlaybackTarget* t) { return t != nullptr && t->lengthSeconds() > 0.0; }

    static void play(PlaybackTarget* t) {
        if (t != nullptr) t->setPaused(false);
    }
    static void pause(PlaybackTarget* t) {
        if (t != nullptr) t->setPaused(true);
    }
    static void stop(PlaybackTarget* t);
    static void seekFraction(PlaybackTarget* t, double fraction) {
        if (live(t)) t->seekSeconds(fraction * t->lengthSeconds());
    }

    struct Shown {
        bool live = false;
        double fraction = 0.0;
        std::string time;
        int paused = -1;
    };

    static Shown shown(PlaybackTarget* t);
};

class VideoPreviewModel {
public:
    VideoPreviewModel(ModelHost& host, std::string organism, std::string previewParam)
        : host_(host), organism_(std::move(organism)), preview_(std::move(previewParam)) {}

    bool previewOn() const { return preview_.empty() || host_.liveParamValue(organism_, preview_) >= 0.5; }
    bool present() const { return host_.liveOrganism(organism_) != nullptr; }
    bool shouldDropFrame() const { return present() && !previewOn(); }
    bool waitingForPicture() const { return present() && previewOn(); }

private:
    ModelHost& host_;
    std::string organism_, preview_;
};

inline constexpr Words kNoLiveInstance{"cam-preview.no-live-instance", "No live instance"};
inline constexpr Words kCameraOff{"cam-preview.camera-off-turn-on-enabled", "Camera off - turn on Enabled below"};
inline constexpr Words kMotion{"cam-preview.motion", "motion "};
inline constexpr Words kBright{"cam-preview.bright", "   bright "};

class CamPreviewModel {
public:
    enum class Show { Picture, Message };

    CamPreviewModel(ModelHost& host, std::string organism) : host_(host), organism_(std::move(organism)) {}

    CamPreviewSource* source() const { return live::source<CamPreviewSource>(host_, organism_); }

    bool active() const;
    std::string message(bool stalled) const;
    bool frameChanged(bool stalled);
    bool producesVideo() const;

    struct Signals {
        float x = 0.5f, y = 0.5f, motion = 0.0f, bright = 0.0f;
        float radius() const { return 8.0f + motion * 26.0f; }
    };

    Signals signals() const;

    std::string signalText(const Signals& s) const {
        return say(host_, kMotion) + decimalText(s.motion, 2) + say(host_, kBright) + decimalText(s.bright, 2);
    }

private:
    ModelHost& host_;
    std::string organism_;
    unsigned lastGen_ = ~0u;
    bool lastActive_ = false;
    bool wasStalled_ = false;
};

}
