// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/clap/ClapTimers.h"

#include <algorithm>
#include <utility>

#include <clap/ext/posix-fd-support.h>

#if JUCE_LINUX || JUCE_BSD
#include <poll.h>
#endif

namespace hum::claphost {

Timers::Timers(Tick tick, FdReady fdReady) : tick_(std::move(tick)), fdReady_(std::move(fdReady)) {}

Timers::~Timers() {
    for (auto& [id, timer] : timers_) timer->stopTimer();
    timers_.clear();
    for (int fd : std::set<int>(fds_)) unwatchFd(fd);
}

bool Timers::add(uint32_t periodMs, clap_id& id) {
    id = next_++;
    auto timer = std::make_unique<One>(tick_, id);
    timer->startTimer((int) std::clamp<uint32_t>(periodMs, 5, 10000));
    timers_[id] = std::move(timer);
    return true;
}

bool Timers::remove(clap_id id) {
    auto it = timers_.find(id);
    if (it == timers_.end()) return false;
    it->second->stopTimer();
    timers_.erase(it);
    return true;
}

#if JUCE_LINUX || JUCE_BSD

bool Timers::watchFd(int fd, uint32_t flags) {
    if (fd < 0) return false;
    short mask = 0;
    if ((flags & CLAP_POSIX_FD_READ) != 0) mask |= POLLIN;
    if ((flags & CLAP_POSIX_FD_WRITE) != 0) mask |= POLLOUT;
    if ((flags & CLAP_POSIX_FD_ERROR) != 0) mask |= POLLERR;
    if (fds_.count(fd) != 0) juce::LinuxEventLoop::unregisterFdCallback(fd);
    juce::LinuxEventLoop::registerFdCallback(fd, [this, flags](int ready) { fdReady_(ready, flags); }, mask);
    fds_.insert(fd);
    return true;
}

bool Timers::unwatchFd(int fd) {
    if (fds_.erase(fd) == 0) return false;
    juce::LinuxEventLoop::unregisterFdCallback(fd);
    return true;
}

#else

bool Timers::watchFd(int, uint32_t) { return false; }
bool Timers::unwatchFd(int) { return false; }

#endif

}
