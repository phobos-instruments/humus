// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <utility>

#include "gui/editor/video/VideoModels.h"
#include "gui/video/VideoLayer.h"

namespace hum {

class JuceVideoTarget : public video::PlaybackTarget {
public:
    explicit JuceVideoTarget(std::shared_ptr<VideoLayer> layer) : layer_(std::move(layer)) {}

    video::PlaybackTarget* get() { return layer_ != nullptr ? this : nullptr; }

    double lengthSeconds() override { return layer_->lengthSeconds(); }
    double positionSeconds() override { return layer_->positionSeconds(); }
    bool isPaused() const override { return layer_->isPaused(); }
    void setPaused(bool paused) override { layer_->setPaused(paused); }
    void seekSeconds(double seconds) override { layer_->seekSeconds(seconds); }

private:
    std::shared_ptr<VideoLayer> layer_;
};

}
