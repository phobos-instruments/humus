// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "FilePlayer/FilePlayer.h"

#include <algorithm>
#include <utility>
#include <memory>

#include "hum/Registry.h"
#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

void FilePlayer::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    loadFromFile({});
    if (hasPending_.load()) applyPending();
    reset();
}

void FilePlayer::reset() {
    readPos_ = 0;
    delayRemaining_ = 0;
    prevActive_ = false;
}

void FilePlayer::loadFromFile(const std::string&) {
    for (int t = 0; t < rows_; ++t) {
        const std::string uri = resolvePath(params.getText(fileParam(t)), "FilePlayer");
        if (uri == loadedUri_[(size_t) t]) continue;
        loadedUri_[(size_t) t] = uri;
        juce::AudioBuffer<float> buf;
        double fileSr = sampleRate_;
        const bool ok = !uri.empty() && loadSoundFile(uri, buf, fileSr);
        if (!ok) buf.setSize(0, 0);
        loadedLen_[(size_t) t] = buf.getNumSamples();
        const juce::ScopedLock sl(loadLock_);
        pending_[(size_t) t] = std::move(buf);
        pendingSet_[(size_t) t] = 1;
        hasPending_.store(true);
    }
    fileSr_.store(sampleRate_);
    fileLen_.store(*std::max_element(loadedLen_.begin(), loadedLen_.end()));
}

void FilePlayer::applyPending() {
    const juce::ScopedTryLock sl(loadLock_);
    if (!sl.isLocked()) return;
    for (int t = 0; t < rows_; ++t) {
        if (!pendingSet_[(size_t) t]) continue;
        std::swap(files_[(size_t) t], pending_[(size_t) t]);
        pendingSet_[(size_t) t] = 0;
    }
    length_ = 0;
    for (const auto& f : files_) length_ = std::max(length_, (int64_t) f.getNumSamples());
    fileLen_.store(length_);
    readPos_ = 0;
    delayRemaining_ = 0;
    hasPending_.store(false);
}

void FilePlayer::renderFrame(int n, float* const* out, int numOut) {
    for (int c = 0; c < numOut; ++c) out[c][n] = 0.0f;
    int covered = 0;
    for (int t = 0; t < rows_; ++t) {
        const int first = t;
        if (first < covered) continue;
        const auto& f = files_[(size_t) t];
        const int fileCh = f.getNumChannels();
        const int width = multi_ ? std::max(1, fileCh) : std::max(kStereo, fileCh);
        covered = first + width;
        if (fileCh == 0 || readPos_ >= f.getNumSamples()) continue;
        for (int c = 0; c < width && first + c < numOut; ++c)
            out[first + c][n] = f.getSample(std::min(c, fileCh - 1), (int) readPos_);
    }
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

    auto silence = [&](int n) { for (int c = 0; c < numOut; ++c) out[c][n] = 0.0f; };

    if (!active || length_ == 0) {
        for (int n = 0; n < numSamples; ++n) silence(n);
        playPos_.store(readPos_);
        return;
    }

    for (int n = 0; n < numSamples; ++n) {
        if (delayRemaining_ > 0) { silence(n); --delayRemaining_; continue; }

        if (readPos_ >= length_) {
            if (!loop) { silence(n); continue; }
            readPos_ = 0;
            if (delaySamples > 0) { delayRemaining_ = delaySamples - 1; silence(n); continue; }
        }

        renderFrame(n, out, numOut);
        ++readPos_;
    }
    playPos_.store(readPos_);
}

}
