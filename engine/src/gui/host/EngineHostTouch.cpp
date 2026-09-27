// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <string>
#include <utility>

#include "core/graph/PerfBox.h"
#include "core/timeline/TouchRelease.h"

namespace hum {

void EngineHost::noteTouch(const std::string& organism, const std::string& param) {
    if (!playing_ || derivedControl_ || noLatch_) return;
    lastTouchMs_[{organism, param}] = juce::Time::getMillisecondCounterHiRes();
    if (!touched_.emplace(organism, param).second) return;
    if (auto* c = model_.byName(organism))
        for (auto& l : c->automation)
            if (l.propertyName == param) { syncAutomation(); return; }
}

void EngineHost::noteFired(const std::string& organism, const std::string& param) {
    lastFiredMs_[{organism, param}] = juce::Time::getMillisecondCounterHiRes();
}

bool EngineHost::firedRecently(const std::string& organism, const std::string& param) const {
    const auto it = lastFiredMs_.find({organism, param});
    return it != lastFiredMs_.end()
           && juce::Time::getMillisecondCounterHiRes() - it->second < kFiredShowsMs;
}

void EngineHost::clearTouches() {
    lastTouchMs_.clear();
    if (touched_.empty()) return;
    touched_.clear();
    syncAutomation();
}

void EngineHost::closeCapturePass(const std::pair<std::string, std::string>& key) {
    const auto it = capturePass_.find(key);
    if (it == capturePass_.end()) return;
    perfbox::addSpan(model_.perfBoxes, key.first, it->second.firstBeat,
                     std::max(it->second.lastBeat, it->second.highBeat));
    capturePass_.erase(it);
    letGoThisCapture_.insert(key);
    dirty_ = true;
}

void EngineHost::releaseQuietTouches(double nowMs) {
    if (latch_ || !playing_ || lastTouchMs_.empty()) return;
    const auto quiet = touchrelease::goneQuiet(lastTouchMs_, nowMs, touchrelease::holdMs(tempo()));
    if (quiet.empty()) return;
    for (const auto& key : quiet) {
        lastTouchMs_.erase(key);
        touched_.erase(key);
        closeCapturePass(key);
    }
    syncAutomation();
    bumpLiveControl();
}

}
