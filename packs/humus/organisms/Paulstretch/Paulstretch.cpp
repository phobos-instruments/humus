// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Paulstretch/Paulstretch.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

void Paulstretch::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    const int maxWin = 1 << PaulstretchFrame::kMaxOrder;
    for (auto& f : frame_) f.prepare(PaulstretchFrame::kMaxOrder);
    for (auto& q : outQ_) q.assign((size_t) (2 * maxWin), 0.0f);
    for (auto& b : halfBuf_) b.assign((size_t) (maxWin / 2), 0.0f);
    loadFromFile(params.getText("File"));
    if (hasPending_.load()) applyPending();
    reset();
}

void Paulstretch::reset() {
    for (auto& f : frame_) f.reset();
    qHead_ = qTail_ = qCount_ = 0;
    srcPos_ = 0.0;
    done_ = false;
    rng_ = 0x2545f491u;
    playPos_.store(0);
}

void Paulstretch::loadFromFile(const std::string& uri) {
    if (uri == loadedUri_) return;
    loadedUri_ = uri;
    juce::AudioBuffer<float> buf;
    double fileSr = sampleRate_;
    const bool ok = loadSoundFile(uri, buf, fileSr);
    const int64_t len = ok ? buf.getNumSamples() : 0;
    if (ok && len > 0) {
        const int fade = std::min((int) len, (int) (sampleRate_ * 0.05));
        for (int c = 0; c < buf.getNumChannels(); ++c)
            buf.applyGainRamp(c, (int) len - fade, fade, 1.0f, 0.0f);
    }
    {
        const juce::ScopedLock sl(loadLock_);
        pending_ = std::move(buf);
    }
    fileSr_.store(sampleRate_);
    fileLen_.store(len);
    hasPending_.store(true);
}

void Paulstretch::applyPending() {
    const juce::ScopedLock sl(loadLock_);
    file_ = std::move(pending_);
    fileLen_.store(file_.getNumSamples());
    srcPos_ = 0.0;
    done_ = false;
    hasPending_.store(false);
}

void Paulstretch::synthFrame(double stretch, bool loop) {
    const int half = frame_[0].half();
    const int64_t len = file_.getNumSamples();
    const int fileCh = file_.getNumChannels();
    const int64_t start = (int64_t) srcPos_;

    for (int c = 0; c < 2; ++c) {
        const int src = std::min(c, fileCh - 1);
        frame_[(size_t) c].synth(file_.getReadPointer(src), len, start, loop, rng_,
                                 halfBuf_[(size_t) c].data());
        auto& q = outQ_[(size_t) c];
        int tail = qTail_;
        for (int i = 0; i < half; ++i) {
            q[(size_t) tail] = halfBuf_[(size_t) c][(size_t) i];
            tail = (tail + 1) % (int) q.size();
        }
    }
    qTail_ = (qTail_ + half) % (int) outQ_[0].size();
    qCount_ += half;

    srcPos_ += (double) frame_[0].win() * 0.5 / std::max(1.0, stretch);
    if (len > 0 && srcPos_ >= (double) len) {
        if (loop) srcPos_ = std::fmod(srcPos_, (double) len);
        else done_ = true;
    }
}

void Paulstretch::process(const float* const*, int,
                          float* const* out, int numOut,
                          int numSamples, const Transport&) {
    if (numOut < 1) return;
    if (hasPending_.load()) applyPending();

    const int64_t seek = seekReq_.exchange(-1);
    if (seek >= 0) {
        srcPos_ = (double) std::min(seek, fileLen_.load());
        done_ = false;
    }

    const bool active = params.get("Active", 1.0) >= 0.5;
    const bool loop = params.get("Loop", 1.0) >= 0.5;
    const double stretch = std::clamp(params.get("Stretch", 8.0), 1.0, 100.0);
    const double windowS = std::clamp(params.get("Window", 0.25), 0.05, 1.0);
    const float level = (float) std::clamp(params.get("Level", 1.0), 0.0, 2.0);
    for (auto& f : frame_)
        f.setOrder(paulstretchOrderForSeconds(windowS, sampleRate_));

    const bool silent = !active || file_.getNumSamples() == 0;
    if (!silent)
        while (qCount_ < numSamples && !done_) synthFrame(stretch, loop);

    for (int n = 0; n < numSamples; ++n) {
        const bool have = qCount_ > 0;
        for (int c = 0; c < numOut; ++c) {
            const auto& q = outQ_[(size_t) std::min(c, 1)];
            out[c][n] = (have && !silent) ? q[(size_t) qHead_] * level : 0.0f;
        }
        if (have && !silent) {
            qHead_ = (qHead_ + 1) % (int) outQ_[0].size();
            --qCount_;
        }
    }
    playPos_.store((int64_t) srcPos_);
}

}
