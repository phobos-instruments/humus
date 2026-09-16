// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "FilePlayer/FilePlayer.h"

#include <algorithm>
#include <utility>
#include <memory>

#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

void FilePlayer::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    loadFromFile(params.getText("File"));
    if (hasPending_.load()) applyPending();
    reset();
}

void FilePlayer::reset() {
    readPos_ = 0;
    delayRemaining_ = 0;
    prevActive_ = false;
}

void FilePlayer::loadFromFile(const std::string& uri) {
    if (uri == loadedUri_) return;
    loadedUri_ = uri;
    juce::AudioBuffer<float> buf;
    double fileSr = sampleRate_;
    const bool ok = loadSoundFile(uri, buf, fileSr);
    const int64_t len = ok ? buf.getNumSamples() : 0;
    {
        const juce::ScopedLock sl(loadLock_);
        pending_ = std::move(buf);
    }
    fileSr_.store(sampleRate_);
    fileLen_.store(len);
    hasPending_.store(true);
}

void FilePlayer::applyPending() {
    const juce::ScopedTryLock sl(loadLock_);
    if (!sl.isLocked()) return;
    std::swap(file_, pending_);
    fileLen_.store(file_.getNumSamples());
    readPos_ = 0;
    delayRemaining_ = 0;
    hasPending_.store(false);
}

void FilePlayer::process(const float* const*, int,
                         float* const* out, int numOut,
                         int numSamples, const Transport&) {
    if (hasPending_.load()) applyPending();

    int64_t seek = seekReq_.exchange(-1);
    if (seek >= 0) { readPos_ = seek; delayRemaining_ = 0; }

    const bool active     = params.get("Active", 1.0) >= 0.5;
    const bool loop       = params.get("Loop", 0.0) >= 0.5;
    const bool autoRewind = params.get("AutoRewind", 0.0) >= 0.5;
    const double delaySec = std::max(0.0, params.get("LoopDelay", 0.0));
    const int64_t delaySamples = (int64_t) (delaySec * sampleRate_ + 0.5);

    if (active && !prevActive_ && autoRewind) { readPos_ = 0; delayRemaining_ = 0; }
    prevActive_ = active;

    const int fileCh = file_.getNumChannels();
    const int64_t len = file_.getNumSamples();

    auto silence = [&](int n) { for (int c = 0; c < numOut; ++c) out[c][n] = 0.0f; };

    if (!active || fileCh == 0 || len == 0) {
        for (int n = 0; n < numSamples; ++n) silence(n);
        playPos_.store(readPos_);
        return;
    }

    for (int n = 0; n < numSamples; ++n) {
        if (delayRemaining_ > 0) { silence(n); --delayRemaining_; continue; }

        if (readPos_ >= len) {
            if (!loop) { silence(n); continue; }
            readPos_ = 0;
            if (delaySamples > 0) { delayRemaining_ = delaySamples - 1; silence(n); continue; }
        }

        for (int c = 0; c < numOut; ++c) {
            int src = std::min(c, fileCh - 1);
            out[c][n] = file_.getSample(src, (int) readPos_);
        }
        ++readPos_;
    }
    playPos_.store(readPos_);
}

}
