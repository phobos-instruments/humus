// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/video/VideoLayer.h"

namespace hum::browser {

class AuditionPicture {
public:
    using MakeLayer = std::function<std::unique_ptr<VideoLayer>()>;

    explicit AuditionPicture(MakeLayer make = {});
    ~AuditionPicture();

    void open(const std::string& path);
    void close();
    const std::string& current() const { return path_; }
    bool ready() const { return layer_ != nullptr; }

    void follow(double seconds, bool rolling);
    bool refresh(int width, int height);
    const juce::Image& picture() const { return picture_; }

    bool waitIdle(int ms);

private:
    struct Arrival {
        unsigned request = 0;
        std::unique_ptr<VideoLayer> layer;
    };

    void adopt();

    MakeLayer make_;
    juce::ThreadPool pool_{1};
    std::mutex lock_;
    std::vector<Arrival> arrivals_;
    std::atomic<unsigned> request_{0};
    std::string path_;
    std::unique_ptr<VideoLayer> layer_;
    std::shared_ptr<const VideoLayer::Frame> shown_;
    juce::Image picture_;
    double lastSeek_ = -1.0;
    float lastRate_ = 0.0f;
};

}
