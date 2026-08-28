#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace hum {

class PitchShifter {
public:
    void prepare(double sampleRate, double windowMs = 60.0) {
        window_ = std::max(64, (int) (sampleRate * windowMs / 1000.0));
        buf_.assign((size_t) (window_ * 2 + 4), 0.0f);
        reset();
    }
    void reset() {
        std::fill(buf_.begin(), buf_.end(), 0.0f);
        writePos_ = 0;
        phase_ = 0.0f;
    }
    void setRatio(double r) { ratio_ = (float) std::clamp(r, 0.25, 4.0); }

    float process(float x) {
        const int size = (int) buf_.size();
        buf_[(size_t) writePos_] = x;

        const float half = window_ * 0.5f;
        const float d1 = phase_;
        float d2 = phase_ + half;
        if (d2 >= window_) d2 -= window_;
        const float pi = 3.14159265358979323846f;
        const float g1 = std::sin(pi * d1 / window_);
        const float g2 = std::sin(pi * d2 / window_);
        const float out = g1 * read(writePos_ - d1) + g2 * read(writePos_ - d2);

        phase_ += (1.0f - ratio_);
        while (phase_ >= window_) phase_ -= window_;
        while (phase_ < 0.0f) phase_ += window_;
        writePos_ = (writePos_ + 1) % size;
        return out;
    }

private:
    float read(float pos) const {
        const int size = (int) buf_.size();
        while (pos < 0.0f) pos += size;
        while (pos >= size) pos -= size;
        const int i0 = (int) pos;
        const int i1 = (i0 + 1) % size;
        const float f = pos - i0;
        return buf_[(size_t) i0] * (1.0f - f) + buf_[(size_t) i1] * f;
    }

    std::vector<float> buf_;
    int window_ = 2646;
    int writePos_ = 0;
    float phase_ = 0.0f;
    float ratio_ = 1.0f;
};

}
