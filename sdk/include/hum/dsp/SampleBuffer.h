// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

namespace hum {

class SampleBuffer {
public:
    void setSize(int channels, int frames) {
        channels_ = channels > 0 ? channels : 0;
        frames_ = frames > 0 ? frames : 0;
        data_.assign((size_t) channels_ * (size_t) frames_, 0.0f);
    }

    int numChannels() const { return channels_; }
    int numFrames() const { return frames_; }
    bool empty() const { return channels_ == 0 || frames_ == 0; }

    float sample(int channel, int frame) const { return data_[index(channel, frame)]; }
    void setSample(int channel, int frame, float value) { data_[index(channel, frame)] = value; }
    void addSample(int channel, int frame, float value) { data_[index(channel, frame)] += value; }
    const float* channel(int c) const { return data_.data() + (size_t) c * (size_t) frames_; }
    float* channel(int c) { return data_.data() + (size_t) c * (size_t) frames_; }

private:
    size_t index(int channel, int frame) const { return (size_t) channel * (size_t) frames_ + (size_t) frame; }

    int channels_ = 0, frames_ = 0;
    std::vector<float> data_;
};

}
