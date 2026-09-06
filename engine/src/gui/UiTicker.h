#pragma once
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include <juce_events/juce_events.h>

namespace hum {

class UiTicker : private juce::Timer {
public:
    static UiTicker& instance() {
        static UiTicker t;
        return t;
    }

    int add(std::function<void()> fn) {
        callbacks_.emplace_back(++lastId_, std::move(fn));
        if (!isTimerRunning()) startTimerHz(30);
        return lastId_;
    }

    void remove(int id) {
        callbacks_.erase(std::remove_if(callbacks_.begin(), callbacks_.end(),
                                        [id](const auto& c) { return c.first == id; }),
                         callbacks_.end());
        if (callbacks_.empty()) stopTimer();
    }

    static int rateHz(bool foreground, bool busy) { return foreground || busy ? 30 : 5; }

    void setBusy(bool busy) {
        if (busy == busy_) return;
        busy_ = busy;
        if (isTimerRunning()) startTimerHz(rateHz(foreground_, busy_));
    }

    bool busy() const { return busy_; }

private:
    void timerCallback() override {
        const bool fg = juce::Process::isForegroundProcess();
        if (fg != foreground_) {
            foreground_ = fg;
            startTimerHz(rateHz(fg, busy_));
        }
        for (size_t i = 0; i < callbacks_.size(); ++i) {
            auto fn = callbacks_[i].second;
            fn();
        }
    }

    std::vector<std::pair<int, std::function<void()>>> callbacks_;
    int lastId_ = 0;
    bool foreground_ = true;
    bool busy_ = false;
};

}
