#pragma once
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class VideoLayer {
public:
    struct Frame {
        int width = 0, height = 0;
        std::vector<unsigned char> bgra;
    };

    virtual ~VideoLayer() = default;
    virtual void load(const juce::String& path) = 0;
    virtual void setRate(float rate) = 0;
    virtual void restart() = 0;
    virtual std::shared_ptr<const Frame> latestFrame() = 0;

    static std::unique_ptr<VideoLayer> create();
};

}
