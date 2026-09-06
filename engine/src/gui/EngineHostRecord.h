#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/VideoTakeRecorder.h"

namespace hum {

class EngineHost;

class RecordHost {
public:
    explicit RecordHost(EngineHost& host) : host_(host) {}

    void toggle();
    bool armed() const;
    bool sessionActive() const { return sessionActive_; }

    void captureToggle();
    bool capturePending() const { return capturing() && !sessionActive_; }

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
    void beginSession();
    void endSession();
    void beginVideoTake(const std::string& node, const juce::File& dir,
                        std::vector<std::string>& existing);
    void endVideoTakes(double samplesPerBeat);
    bool recordArmed(const std::string& node) const;
    std::vector<std::string> autoArmed_;
    void setArmed(bool on);

    EngineHost& host_;
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
