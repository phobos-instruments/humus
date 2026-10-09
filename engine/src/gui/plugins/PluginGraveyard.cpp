// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/PluginGraveyard.h"

#include <algorithm>
#include <utility>

#include "core/plugins/PluginHost.h"

namespace hum {

PluginGraveyard::PluginGraveyard(int graceMs) : graceMs_(graceMs) {}

PluginGraveyard::~PluginGraveyard() {
    alive_.reset();
    stopTimer();
    release(true);
}

void PluginGraveyard::bury(std::unique_ptr<juce::AudioPluginInstance> instance, const std::string& classRaw) {
    if (instance == nullptr) return;
    {
        const std::lock_guard<std::mutex> held(lock_);
        graves_.push_back({std::move(instance), classRaw, juce::Time::getMillisecondCounterHiRes()});
    }
    if (juce::MessageManager::existsAndIsCurrentThread()) {
        wake();
        return;
    }
    juce::MessageManager::callAsync([this, alive = std::weak_ptr<bool>(alive_)] {
        if (alive.lock()) wake();
    });
}

void PluginGraveyard::flush() {
    stopTimer();
    release(true);
}

int PluginGraveyard::waiting() const {
    const std::lock_guard<std::mutex> held(lock_);
    return (int) graves_.size();
}

void PluginGraveyard::wake() {
    if (!isTimerRunning()) startTimer(std::max(10, graceMs_ / 4));
}

void PluginGraveyard::timerCallback() {
    release(false);
    if (waiting() == 0) stopTimer();
}

void PluginGraveyard::release(bool all) {
    std::vector<Grave> due;
    {
        const std::lock_guard<std::mutex> held(lock_);
        const double now = juce::Time::getMillisecondCounterHiRes();
        for (auto it = graves_.begin(); it != graves_.end();) {
            if (all || now - it->buriedAtMs >= (double) graceMs_) {
                due.push_back(std::move(*it));
                it = graves_.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& grave : due) PluginHost::destroyNow(std::move(grave.instance), grave.classRaw);
}

}
