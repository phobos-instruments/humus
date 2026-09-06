#pragma once
#include <memory>
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

    static std::unique_ptr<VideoLayer> create();
    static std::unique_ptr<VideoLayer> createPlatform();
};

}
