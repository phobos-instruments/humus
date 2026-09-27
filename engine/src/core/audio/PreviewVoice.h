// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "hum/dsp/Prepared.h"

namespace hum {

struct PreviewSound {
    std::vector<float> left, right;
    double sampleRate = 0.0;
    std::int64_t frames() const { return (std::int64_t) left.size(); }
};

class PreviewVoice {
public:
    static constexpr float kEdgeMs = 4.0f;

    void play(std::shared_ptr<const PreviewSound> sound) {
        generation_.fetch_add(1, std::memory_order_relaxed);
        pending_.publish(std::move(sound));
    }
    void stop() { play(nullptr); }

    void seek(double seconds) { seekTo_.store(std::max(0.0, seconds), std::memory_order_relaxed); }
    void setLoop(bool on) { loop_.store(on, std::memory_order_relaxed); }
    void setGain(float g) { gain_.store(std::clamp(g, 0.0f, 2.0f), std::memory_order_relaxed); }
    void setSpeed(double s) { speed_.store(std::clamp(s, 0.25, 4.0), std::memory_order_relaxed); }
    bool loop() const { return loop_.load(std::memory_order_relaxed); }
    float gain() const { return gain_.load(std::memory_order_relaxed); }

    double position() const { return position_.load(std::memory_order_relaxed); }
    bool playing() const { return playing_.load(std::memory_order_relaxed); }
    unsigned generation() const { return generation_.load(std::memory_order_relaxed); }

    void render(float* left, float* right, int n, double deviceRate) {
        if (pending_.adopt(live_)) {
            pos_ = 0.0;
            fadeIn_ = 0.0f;
            finished_ = false;
        }
        const auto* s = live_.get();
        if (s == nullptr || finished_ || s->frames() <= 1 || deviceRate <= 0.0) {
            playing_.store(false, std::memory_order_relaxed);
            return;
        }
        if (const double to = seekTo_.exchange(-1.0, std::memory_order_relaxed); to >= 0.0) {
            pos_ = std::min(to * s->sampleRate, (double) s->frames() - 2.0);
            fadeIn_ = 0.0f;
        }
        const double step = s->sampleRate / deviceRate * speed_.load(std::memory_order_relaxed);
        const float gain = gain_.load(std::memory_order_relaxed);
        const bool looping = loop_.load(std::memory_order_relaxed);
        const auto frames = (double) s->frames();
        const float edge = 1.0f / std::max(1.0f, kEdgeMs * 0.001f * (float) deviceRate);
        for (int i = 0; i < n; ++i) {
            if (pos_ >= frames - 1.0) {
                if (!looping) {
                    finished_ = true;
                    break;
                }
                pos_ = std::fmod(pos_, frames - 1.0);
            }
            const auto a = (std::int64_t) pos_;
            const float fr = (float) (pos_ - (double) a);
            const float l = s->left[(size_t) a] + (s->left[(size_t) a + 1] - s->left[(size_t) a]) * fr;
            const float r = s->right[(size_t) a] + (s->right[(size_t) a + 1] - s->right[(size_t) a]) * fr;
            fadeIn_ = std::min(1.0f, fadeIn_ + edge);
            left[i] += l * gain * fadeIn_;
            if (right != left) right[i] += r * gain * fadeIn_;
            pos_ += step;
        }
        playing_.store(!finished_, std::memory_order_relaxed);
        position_.store(finished_ ? 0.0 : pos_ / s->sampleRate, std::memory_order_relaxed);
    }

private:
    Prepared<std::shared_ptr<const PreviewSound>> pending_;
    std::shared_ptr<const PreviewSound> live_;
    double pos_ = 0.0;
    float fadeIn_ = 0.0f;
    bool finished_ = false;
    std::atomic<bool> loop_{false};
    std::atomic<float> gain_{0.8f};
    std::atomic<double> speed_{1.0};
    std::atomic<double> position_{0.0};
    std::atomic<double> seekTo_{-1.0};
    std::atomic<bool> playing_{false};
    std::atomic<unsigned> generation_{0};
};

}
