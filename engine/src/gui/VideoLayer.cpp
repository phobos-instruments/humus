#include "gui/VideoLayer.h"

#include "core/HapFile.h"
#include "gui/VideoLayerHap.h"

namespace hum {

#if !JUCE_MAC && !defined(HUM_FFMPEG) && !defined(HUM_MEDIA_FOUNDATION)
std::unique_ptr<VideoLayer> VideoLayer::createPlatform() { return nullptr; }
#endif

namespace {

class SwitchingLayer : public VideoLayer {
public:
    void load(const juce::String& path) override {
        const bool wantHap = hap::open(juce::File(path)).ok;
        if (wantHap != isHap_ || inner_ == nullptr) {
            inner_ = wantHap ? std::make_unique<HapVideoLayer>()
                             : VideoLayer::createPlatform();
            isHap_ = wantHap;
        }
        if (inner_ != nullptr) inner_->load(path);
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

private:
    std::unique_ptr<VideoLayer> inner_;
    bool isHap_ = false;
};

}

std::unique_ptr<VideoLayer> VideoLayer::create() {
    return std::make_unique<SwitchingLayer>();
}

}
