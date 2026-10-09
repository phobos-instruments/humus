// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "gui/host/HostAudioIo.h"

namespace hum {

namespace {

int deviceDropoutsOf(HostAudioIo& io) {
    auto* device = io.current();
    return device != nullptr ? device->getXRunCount() : -1;
}

}

unsigned EngineHost::glitchCount() const {
    return dropouts_.load(std::memory_order_relaxed) + lateBlocks_.load(std::memory_order_relaxed)
         + deviceGlitches_;
}

AudioGlitchWindow EngineHost::takeAudioGlitches() {
    AudioGlitchWindow window;
    const unsigned silent = dropouts_.load(std::memory_order_relaxed);
    const unsigned late = lateBlocks_.load(std::memory_order_relaxed);
    window.silentBlocks = silent - seenSilent_;
    window.lateBlocks = late - seenLate_;
    seenSilent_ = silent;
    seenLate_ = late;

    const int deviceDropouts = deviceDropoutsOf(*audio_);
    if (deviceDropouts >= 0 && seenDeviceDropouts_ >= 0 && deviceDropouts >= seenDeviceDropouts_) {
        window.deviceDropouts = deviceDropouts - seenDeviceDropouts_;
        deviceGlitches_ += (unsigned) window.deviceDropouts;
    }
    seenDeviceDropouts_ = deviceDropouts;

    window.loadPeak = loadPeak_.exchange(0.0f, std::memory_order_relaxed);
    const auto hold = lock_.takeLongestHold();
    window.longestHoldMs = hold.ms;
    window.holdSite = codeSite(hold.file, hold.line);
    window.blockSize = block_;
    window.sampleRate = sampleRate_;
    return window;
}

}
