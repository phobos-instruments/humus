#pragma once
#include <algorithm>
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
};

inline std::pair<double, double> loopWindow(double in, double out, double span) {
    const double lo = span > 0.0 && in >= span ? 0.0 : std::max(0.0, in);
    const double hi = out > lo && (span <= 0.0 || out < span) ? out : span;
    return {lo, hi};
}

}
