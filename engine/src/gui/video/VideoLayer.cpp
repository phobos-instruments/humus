// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VideoLayer.h"

#include "core/video/HapFile.h"
#include "gui/video/VideoLayerHap.h"

namespace hum {

#if !JUCE_MAC && !defined(HUM_FFMPEG) && !defined(HUM_MEDIA_FOUNDATION)
std::unique_ptr<VideoLayer> VideoLayer::createPlatform() { return nullptr; }
#endif

#if !JUCE_MAC
double VideoLayer::probeLengthSeconds(const juce::File&) { return 0.0; }
std::unique_ptr<VideoLayer> VideoLayer::createOffline() { return createPlatform(); }
#endif

namespace {

class SwitchingLayer : public VideoLayer {
public:
    explicit SwitchingLayer(bool offline) : offline_(offline) {}

    void load(const juce::String& path) override {
        const bool wantHap = hap::open(juce::File(path)).ok;
        if (wantHap != isHap_ || inner_ == nullptr) {
            inner_ = wantHap    ? std::make_unique<HapVideoLayer>()
                     : offline_ ? VideoLayer::createOffline()
                                : VideoLayer::createPlatform();
            isHap_ = wantHap;
        }
        if (inner_ != nullptr) {
            inner_->setLoopRange(range_);
            inner_->load(path);
        }
    }

    void setLoopRange(const LoopRange& range) override {
        range_ = range;
        if (inner_ != nullptr) inner_->setLoopRange(range);
    }

    void setRate(float rate) override {
        if (inner_ != nullptr) inner_->setRate(rate);
    }

    void restart() override {
        if (inner_ != nullptr) inner_->restart();
    }

    std::shared_ptr<const Frame> latestFrame() override {
        return inner_ != nullptr ? inner_->latestFrame() : nullptr;
    }

    void setPaused(bool paused) override {
        if (inner_ != nullptr) inner_->setPaused(paused);
    }

    bool isPaused() const override { return inner_ != nullptr && inner_->isPaused(); }

    double positionSeconds() override {
        return inner_ != nullptr ? inner_->positionSeconds() : 0.0;
    }

    double lengthSeconds() override {
        return inner_ != nullptr ? inner_->lengthSeconds() : 0.0;
    }

    void seekSeconds(double t) override {
        if (inner_ != nullptr) inner_->seekSeconds(t);
    }

    void chase(double seconds, double rate) override {
        if (inner_ != nullptr) inner_->chase(seconds, rate);
    }

private:
    std::unique_ptr<VideoLayer> inner_;
    LoopRange range_;
    bool isHap_ = false;
    const bool offline_;
};

}

std::unique_ptr<VideoLayer> VideoLayer::create(bool offline) {
    return std::make_unique<SwitchingLayer>(offline);
}

}
