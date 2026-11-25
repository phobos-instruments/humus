// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/AuditionPicture.h"

#include <utility>

#include "gui/video/FrameImage.h"

namespace hum::browser {

AuditionPicture::AuditionPicture(MakeLayer make)
    : make_(make ? std::move(make) : MakeLayer([] { return VideoLayer::create(); })) {}

AuditionPicture::~AuditionPicture() { pool_.removeAllJobs(true, 4000); }

void AuditionPicture::open(const std::string& path) {
    if (path == path_ && !path.empty()) return;
    close();
    path_ = path;
    const unsigned mine = request_.load();
    pool_.addJob([this, path, mine] {
        if (request_.load() != mine) return;
        auto layer = make_();
        if (layer == nullptr) return;
        layer->load(juce::String(path));
        layer->setRate(0.0f);
        const std::lock_guard<std::mutex> hold(lock_);
        arrivals_.push_back({mine, std::move(layer)});
    });
}

void AuditionPicture::close() {
    ++request_;
    path_.clear();
    layer_.reset();
    shown_.reset();
    picture_ = {};
    lastSeek_ = -1.0;
    lastRate_ = 0.0f;
}

void AuditionPicture::adopt() {
    std::vector<Arrival> landed;
    {
        const std::lock_guard<std::mutex> hold(lock_);
        landed.swap(arrivals_);
    }
    for (auto& a : landed)
        if (a.request == request_.load() && !path_.empty()) {
            layer_ = std::move(a.layer);
            lastSeek_ = -1.0;
        }
}

void AuditionPicture::follow(double seconds, bool rolling) {
    adopt();
    if (layer_ == nullptr) return;
    const auto step = followStep(lastSeek_, lastRate_, seconds, rolling ? 1.0f : 0.0f);
    if (!step.chase) return;
    lastSeek_ = step.seconds;
    lastRate_ = step.rate;
    layer_->chase(step.seconds, step.rate);
}

bool AuditionPicture::refresh(int width, int height) {
    adopt();
    if (layer_ == nullptr || width <= 0 || height <= 0) return false;
    auto frame = layer_->latestFrame();
    if (frame == nullptr || frame == shown_ || frame->width <= 0 || frame->height <= 0) return false;
    shown_ = frame;
    const auto fit = juce::RectanglePlacement(juce::RectanglePlacement::centred)
                         .appliedTo(juce::Rectangle<int>(frame->width, frame->height),
                                    juce::Rectangle<int>(width, height));
    if (auto img = imageOfFrame(*frame, fit.getWidth(), fit.getHeight()); img.isValid()) picture_ = std::move(img);
    return true;
}

bool AuditionPicture::waitIdle(int ms) {
    const auto until = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
    while (pool_.getNumJobs() > 0) {
        if (juce::Time::getMillisecondCounter() > until) return false;
        juce::Thread::sleep(2);
    }
    return true;
}

}
