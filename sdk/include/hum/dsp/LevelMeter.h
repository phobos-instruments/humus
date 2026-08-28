#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

namespace hum {

class LevelMeter {
public:
    static constexpr int kMax = 32;

    void prepare(double sampleRate) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        reset();
    }
    void reset() {
        for (auto& m : levels_) m.store(0.0f, std::memory_order_relaxed);
    }

    void measure(const float* const* buf, int channels, int numSamples) {
        const float rel = std::exp(-(float) numSamples / (0.3f * (float) sampleRate_));
        for (int c = 0; c < channels && c < kMax; ++c) {
            float peak = 0.0f;
            if (buf && buf[c])
                for (int i = 0; i < numSamples; ++i) peak = std::max(peak, std::fabs(buf[c][i]));
            float cur = levels_[(size_t) c].load(std::memory_order_relaxed);
            cur = peak > cur ? peak : cur * rel;
            levels_[(size_t) c].store(cur, std::memory_order_relaxed);
        }
    }

    float level(int channel) const {
        return channel >= 0 && channel < kMax
                   ? levels_[(size_t) channel].load(std::memory_order_relaxed)
                   : 0.0f;
    }

private:
    std::array<std::atomic<float>, kMax> levels_{};
    double sampleRate_ = 44100.0;
};

}
