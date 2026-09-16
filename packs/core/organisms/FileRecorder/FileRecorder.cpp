// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "FileRecorder/FileRecorder.h"

#include <algorithm>
#include <cstdint>

namespace hum {

void FileRecorder::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    meter_.prepare(sampleRate);
    zero_.assign((size_t) std::max(1, maxBlock), 0.0f);
    writePtrs_.assign((size_t) channels_, nullptr);
}

bool FileRecorder::startRecording(const std::vector<RecordTarget>& targets, int punchMode,
                                  double durationSeconds, double sampleRate, bool append) {
    stopRecording();
    punchMode_ = punchMode;
    recorded_ = 0;
    limitSamples_ = (punchMode == 2 && durationSeconds > 0.0)
                        ? (std::int64_t) (durationSeconds * sampleRate) : 0;
    autoStop_.store(false);

    int base = 0;
    bool any = false;
    for (const auto& t : targets) {
        const int nch = std::max(1, t.channels);
        auto w = std::make_unique<LiveWavWriter>();
        if (w->start(t.path, nch, sampleRate, append)) {
            writers_.push_back(std::move(w));
            firstChannel_.push_back(base);
            writerChannels_.push_back(nch);
            any = true;
        }
        base += nch;
    }
    if ((int) writePtrs_.size() < base) writePtrs_.assign((size_t) base, nullptr);
    return any;
}

void FileRecorder::stopRecording() {
    for (auto& w : writers_) w->stop();
    writers_.clear();
    firstChannel_.clear();
    writerChannels_.clear();
    recorded_ = 0;
    limitSamples_ = 0;
}

bool FileRecorder::isRecording() const {
    for (auto& w : writers_) if (w->active()) return true;
    return false;
}

void FileRecorder::process(const float* const* in, int numIn,
                           float* const* out, int numOut,
                           int numSamples, const Transport&) {
    meter_.measure(in, std::min(numIn, channels_), numSamples);
    if (!writers_.empty()) {
        const bool overLimit = limitSamples_ > 0 && recorded_ >= limitSamples_;
        if (!overLimit) {
            for (size_t w = 0; w < writers_.size(); ++w) {
                const int base = firstChannel_[w];
                const int n = writerChannels_[w];
                for (int c = 0; c < n; ++c) {
                    const int src = base + c;
                    writePtrs_[(size_t) c] = (src < numIn && in && in[src]) ? in[src] : zero_.data();
                }
                writers_[w]->write(writePtrs_.data(), numSamples);
            }
            recorded_ += numSamples;
            if (limitSamples_ > 0 && recorded_ >= limitSamples_) autoStop_.store(true);
        }
    }
    for (int c = 0; c < numOut; ++c) {
        if (c < numIn && in && in[c]) std::copy(in[c], in[c] + numSamples, out[c]);
        else std::fill(out[c], out[c] + numSamples, 0.0f);
    }
}

}
