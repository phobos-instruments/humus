// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "gui/video/VideoTakeRecorder.h"

namespace hum {


class RecordHost {
public:
    RecordHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), audio_(core), nodes_(core), capture_(core), recording_(core) {}

    void toggle();
    bool armed() const;
    bool sessionActive() const { return sessionActive_; }

    void captureToggle();
    bool capturePending() const { return capturing() && !sessionActive_; }
    bool preRolling() const;
    void onPunchIn();
    void clearPending() { pendingCapture_ = pendingArm_ = false; }

    void setCapturing(bool on);
    bool capturing() const;

    void onPlay();
    void onStop();

    juce::File recordingsDir() const;
    int videoTakesActive() const { return (int) videoTakes_.size(); }
    double videoTakeStartBeat(const std::string& node) const {
        for (const auto& t : videoTakes_)
            if (t.node == node && t.recorder->frames() > 0) return t.recorder->takeStartBeat();
        return -1.0;
    }

    std::function<void()> onSessionEnded;
    std::function<void()> onStartRolling;

private:
    void startRolling(bool capture);
    void beginSession();
    bool pendingCapture_ = false;
    bool pendingArm_ = false;
    void endSession();
    void beginVideoTake(const std::string& node, const juce::File& dir,
                        std::vector<std::string>& existing);
    void endVideoTakes(double samplesPerBeat);
    bool recordArmed(const std::string& node) const;
    std::vector<std::string> autoArmed_;
    void setArmed(bool on);

    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
    HostNodes& nodes_;
    HostCapture& capture_;
    HostRecording& recording_;
    bool sessionActive_ = false;
    struct ActiveTake { std::string node; std::string path; };
    std::vector<ActiveTake> takes_;
    struct VideoTake {
        std::string node;
        std::shared_ptr<VideoTakeRecorder> recorder;
    };
    std::vector<VideoTake> videoTakes_;
};

}
