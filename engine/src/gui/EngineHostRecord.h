#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

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

    std::function<void()> onSessionEnded;
    std::function<void()> onStartRolling;

private:
    void beginSession();
    void endSession();
    std::vector<std::string> autoArmed_;
    void setArmed(bool on);

    EngineHost& host_;
    bool sessionActive_ = false;
    struct ActiveTake { std::string node; std::string path; };
    std::vector<ActiveTake> takes_;
};

}
