#pragma once
#include <memory>

#include <juce_core/juce_core.h>

#include "core/HapFile.h"
#include "gui/VideoLayer.h"

namespace hum {

inline bool isVideoFile(const juce::File& f) {
    return f.hasFileExtension("mov;mp4;m4v;avi;mkv;webm;wmv;mpg;mpeg");
}

inline double probeVideoSeconds(const juce::File& f, int waitMs = 1500) {
    if (const auto movie = hap::open(f); movie.ok && movie.fps > 0.0)
        return (double) movie.samples.size() / movie.fps;
    if (const double s = VideoLayer::probeLengthSeconds(f); s > 0.0) return s;
    auto layer = VideoLayer::createPlatform();
    if (layer == nullptr) return 0.0;
    layer->setPaused(true);
    layer->load(f.getFullPathName());
    const auto until = juce::Time::getMillisecondCounter() + (juce::uint32) waitMs;
    while (juce::Time::getMillisecondCounter() < until) {
        if (const double s = layer->lengthSeconds(); s > 0.0) return s;
        juce::Thread::sleep(10);
    }
    return layer->lengthSeconds();
}

}
