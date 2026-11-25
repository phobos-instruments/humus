// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>

#include <juce_events/juce_events.h>

#include <clap/id.h>

namespace hum::claphost {

class Timers {
public:
    using Tick = std::function<void(clap_id)>;
    using FdReady = std::function<void(int fd, uint32_t flags)>;

    Timers(Tick tick, FdReady fdReady);
    ~Timers();

    bool add(uint32_t periodMs, clap_id& id);
    bool remove(clap_id id);
    bool watchFd(int fd, uint32_t flags);
    bool unwatchFd(int fd);
    int count() const { return (int) timers_.size(); }

private:
    struct One : juce::Timer {
        One(Tick& t, clap_id i) : tick(t), id(i) {}
        void timerCallback() override { tick(id); }
        Tick& tick;
        clap_id id;
    };

    Tick tick_;
    FdReady fdReady_;
    std::map<clap_id, std::unique_ptr<One>> timers_;
    std::set<int> fds_;
    clap_id next_ = 1;
};

}
