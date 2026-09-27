// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/HostScheduler.h"

#include <utility>

#include <juce_events/juce_events.h>

namespace hum {

namespace {

class TimerRepeat final : public HostRepeat, private juce::Timer {
public:
    TimerRepeat(int intervalMs, std::function<void()> tick) : tick_(std::move(tick)) {
        startTimer(intervalMs);
    }
    ~TimerRepeat() override { stopTimer(); }

private:
    void timerCallback() override { tick_(); }
    std::function<void()> tick_;
};

class MessageThreadScheduler final : public HostScheduler {
public:
    void post(std::function<void()> job) override {
        juce::MessageManager::callAsync(std::move(job));
    }
    std::unique_ptr<HostRepeat> repeat(int intervalMs, std::function<void()> tick) override {
        return std::make_unique<TimerRepeat>(intervalMs, std::move(tick));
    }
};

}

HostScheduler& messageThreadScheduler() {
    static MessageThreadScheduler scheduler;
    return scheduler;
}

}
