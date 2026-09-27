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
    void noteRecordSwitch(const std::string& node, bool on);
    void noteTransport(const std::string& node, const std::string& param, double value);

    juce::File recordingsDir() const;
    int videoTakesActive() const { return (int) videoTakes_.size(); }
    int looseTakesActive() const { return (int) loose_.size(); }
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
    void armWiredTracks();
    bool pendingCapture_ = false;
    bool pendingArm_ = false;
    void endSession();
    static constexpr int kTakeNameTries = 9999;
    juce::File freeTakeFile(const juce::File& dir, const std::string& node, const std::string& ext,
                            std::vector<std::string>& existing);
    void beginVideoTake(const std::string& node, const juce::File& dir,
                        std::vector<std::string>& existing);
    std::shared_ptr<VideoTakeRecorder> openVideoTake(const std::string& node,
                                                     const juce::File& dir,
                                                     std::vector<std::string>& existing,
                                                     bool freeRunning);
    bool recordsItsOwnPicture(const std::string& node) const;
    void finishLooseTake(const std::string& node);
    bool rollingLoose(const std::string& node) const;
    void sweepEmptyTakeFolders() const;
    void endVideoTakes(double samplesPerBeat);
    std::vector<std::string> pictured_;
    bool recordArmed(const std::string& node) const;
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
        std::string sound;
    };
    std::vector<VideoTake> videoTakes_;
    std::vector<VideoTake> loose_;
    mutable juce::String unsaved_;
};

}
