// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "LFO/LfoTable.h"

#include <algorithm>
#include <cmath>

namespace hum {

namespace {

constexpr double kGlideSeconds = 0.008;

}

std::unique_ptr<LfoTable::Frames> LfoTable::decode(const std::string& text) {
    auto frames = std::make_unique<Frames>();
    const int n = decodeWaveFrames(text.c_str(), &frames->data[0][0], kWaveTableLen, kMaxFrames);
    if (n > 0) {
        frames->count = n;
        waveTableAlignFrames(&frames->data[0][0], frames->count, kWaveTableLen);
    } else {
        waveTablePreset(0, frames->data[0], kWaveTableLen);
        frames->count = 1;
    }
    return frames;
}

void LfoTable::sync(const std::string& text) {
    if (frames_ != nullptr && text == applied_) return;
    applied_ = text;
    frames_ = decode(text);
}

void LfoTable::publish(const std::string& text) {
    applied_ = text;
    pending_.publish(decode(text));
}

void LfoTable::adopt() { pending_.adopt(frames_); }

void LfoTable::prepare(double sampleRate) {
    glide_ = 1.0 - std::exp(-1.0 / (kGlideSeconds * sampleRate));
    position_ = target_;
}

double LfoTable::at(double phase) {
    if (frames_ == nullptr) return 0.0;
    position_ += glide_ * (target_ - position_);
    const double fp = std::clamp(position_, 0.0, 1.0) * (frames_->count - 1);
    const int f0 = std::clamp((int) fp, 0, frames_->count - 1);
    const int f1 = std::min(f0 + 1, frames_->count - 1);
    const float mix = (float) (fp - f0);
    const float a = waveTableRead(frames_->data[f0], kWaveTableLen, phase);
    const float b = waveTableRead(frames_->data[f1], kWaveTableLen, phase);
    return a + mix * (b - a);
}

void LfoTable::publishCycle() {
    if (frames_ == nullptr) return;
    const double fp = std::clamp(position_, 0.0, 1.0) * (frames_->count - 1);
    const int f0 = std::clamp((int) fp, 0, frames_->count - 1);
    const int f1 = std::min(f0 + 1, frames_->count - 1);
    const float mix = (float) (fp - f0);
    for (int i = 0; i < MotionTrace::kPoints; ++i) {
        const double phase = (double) i / (MotionTrace::kPoints - 1);
        const float a = waveTableRead(frames_->data[f0], kWaveTableLen, phase);
        const float b = waveTableRead(frames_->data[f1], kWaveTableLen, phase);
        cycle_[(size_t) i].store(a + mix * (b - a), std::memory_order_relaxed);
    }
}

int LfoTable::readCycle(MotionTrace::Point* out, int capacity) const {
    const int n = std::min(capacity, MotionTrace::kPoints);
    for (int i = 0; i < n; ++i) {
        const float v = cycle_[(size_t) i].load(std::memory_order_relaxed);
        out[i] = {v, v};
    }
    return n;
}

}
