// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class VideoLayer {
public:
    struct Frame {
        enum Fmt { BGRA, DXT1, DXT5, YCoCgDXT5 };
        int width = 0, height = 0;
        int fmt = BGRA;
        std::vector<unsigned char> bgra;
        std::vector<unsigned char> blocks;
        void* native = nullptr;
        std::shared_ptr<const void> nativeHold;
        std::function<bool(std::vector<unsigned char>&)> readPixels;
        double pts = -1.0;
    };

    virtual ~VideoLayer() = default;
    virtual void load(const juce::String& path) = 0;
    virtual void setRate(float rate) = 0;
    virtual void restart() = 0;
    virtual std::shared_ptr<const Frame> latestFrame() = 0;
    virtual void setPaused(bool) {}
    virtual bool isPaused() const { return false; }
    virtual double positionSeconds() { return 0.0; }
    virtual double lengthSeconds() { return 0.0; }
    virtual void seekSeconds(double) {}
    struct LoopRange {
        double in = 0.0;
        double out = 0.0;
        bool loop = true;
    };
    virtual void setLoopRange(const LoopRange&) {}
    virtual void chase(double seconds, double rate) {
        seekSeconds(seconds);
        setRate((float) rate);
    }

    static std::unique_ptr<VideoLayer> create(bool offline = false);
    static std::unique_ptr<VideoLayer> createPlatform();
    static std::unique_ptr<VideoLayer> createOffline();
    static double probeLengthSeconds(const juce::File& file);
    static std::unique_ptr<VideoLayer> createFfmpeg();
    static bool systemCanPlay(const juce::File& file);
};

inline constexpr double kFollowSlack = 1.0 / 120.0;

struct FollowStep {
    bool chase = false;
    double seconds = 0.0;
    float rate = 0.0f;
};

inline constexpr std::size_t kScrubRingBytes = 64u * 1024u * 1024u;
inline constexpr double kScrubRingSeconds = 2.0;
inline constexpr double kScrubRingReach = 0.25;

inline bool ringHolds(std::size_t frames, std::size_t bytes, double span) {
    return frames <= 1 || (bytes <= kScrubRingBytes && span <= kScrubRingSeconds);
}

inline int heldFrameFor(const std::vector<double>& times, double target, double reach) {
    int best = -1;
    for (std::size_t i = 0; i < times.size(); ++i) {
        if (times[i] > target + 1.0e-6) continue;
        if (best < 0 || times[i] > times[(std::size_t) best]) best = (int) i;
    }
    if (best < 0) return -1;
    return target - times[(std::size_t) best] <= reach ? best : -1;
}

inline double pictureLagSeconds(int block, int latencySamples, double sampleRate) {
    if (sampleRate <= 0.0) return 0.0;
    const int behind = block + latencySamples;
    return behind <= 0 ? 0.0 : (double) behind / sampleRate;
}

inline FollowStep followStep(double lastSeconds, float lastRate, double seconds, float rate) {
    FollowStep s;
    s.seconds = seconds < 0.0 ? 0.0 : seconds;
    s.rate = rate;
    s.chase = std::abs(s.seconds - lastSeconds) > kFollowSlack
              || std::abs(rate - lastRate) > 1.0e-3f;
    return s;
}

inline std::pair<double, double> loopWindow(double in, double out, double span) {
    const double lo = span > 0.0 && in >= span ? 0.0 : std::max(0.0, in);
    const double hi = out > lo && (span <= 0.0 || out < span) ? out : span;
    return {lo, hi};
}

}
