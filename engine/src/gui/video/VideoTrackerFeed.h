// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gui/host/VideoHost.h"
#include "gui/common/UiTicker.h"
#include "gui/video/VideoDeckPool.h"
#include "gui/video/VideoRenderService.h"
#include "gui/video/VideoPreviewStore.h"
#include "gui/video/VisualPlanBuilder.h"
#include "hum/caps/Video.h"

namespace hum {

class VideoTrackerFeed {
public:
    static constexpr int kFeedW = 512, kFeedH = 288;

    VideoTrackerFeed(VideoHost& host, std::string sink,
                     VideoRenderService* renderer = nullptr)
        : host_(host), sink_(std::move(sink)), renderer_(renderer),
          builder_(host_, sink_, true) {
        VideoPreviewStore::instance().want(sink_, true);
        attach(true);
        tickerId_ = UiTicker::instance().add([this] { tick(); });
    }

    ~VideoTrackerFeed() {
        UiTicker::instance().remove(tickerId_);
        wantComposite(false);
        attach(false);
        VideoPreviewStore::instance().want(sink_, false);
    }

    void tick() {
        auto* dst = dynamic_cast<VideoFrameSink*>(host_.liveOrganism(sink_));
        if (dst == nullptr) return;
        dst->setVideoCordAttached(true);
        if (auto* cam = directCamera()) {
            wantComposite(false);
            dst->setVideoSourceHeld(false);
            const unsigned gen = cam->camGeneration();
            if (gen == camGen_) return;
            camGen_ = gen;
            if (const auto np = cam->camNativePicture(); np.buffer != nullptr) {
                VideoFrameSink::Picture pic;
                pic.width = np.width;
                pic.height = np.height;
                pic.native = np.buffer;
                pic.hold = np.hold;
                pic.mirrored = np.mirrored;
                dst->pushVideoFrame(pic);
                return;
            }
            auto held = std::make_shared<CamPreviewSource::Frame>(cam->camFrame());
            if (held->width > 0 && held->height > 0
                && held->rgba.size()
                       >= (size_t) held->width * (size_t) held->height * 4u)
                dst->pushVideoFrame({held->width, held->height, false,
                                     held->rgba.data(), nullptr, held});
            return;
        }
        const auto plan = builder_.build();
        const visual::Step* root =
            plan.root >= 0 && plan.root < (int) plan.steps.size()
                ? &plan.steps[(size_t) plan.root]
                : nullptr;
        if (root == nullptr) {
            wantComposite(false);
            return;
        }
        if (root->kind == visual::Step::Deck) {
            bool held = false;
            if (const auto lay = VideoDeckPool::instance().peek(root->node)) {
                float rate = 1.0f;
                if (const auto* cm = host_.model().byName(root->node))
                    for (const auto& prop : cm->properties)
                        if (prop.name == "Rate") rate = (float) prop.value;
                held = lay->isPaused() || std::abs(rate) < 1.0e-3f;
            }
            dst->setVideoSourceHeld(held);
            if (root->frame == nullptr) {
                wantComposite(false);
                return;
            }
            const auto& f = *root->frame;
            if (f.native != nullptr || !f.bgra.empty()) {
                wantComposite(false);
                if (root->frame.get() == lastDeck_) return;
                lastDeck_ = root->frame.get();
                dst->pushVideoFrame({f.width, f.height, true,
                                     f.bgra.empty() ? nullptr : f.bgra.data(),
                                     f.native, root->frame});
                return;
            }
        }
        wantComposite(true);
        dst->setVideoSourceHeld(false);
        auto held = std::make_shared<std::vector<std::uint8_t>>();
        int w = 0, h = 0;
        if (VideoPreviewStore::instance().takeRaw(sink_, gen_, w, h, *held)
            && w > 0 && h > 0)
            dst->pushVideoFrame({w, h, false, held->data(), nullptr, held});
    }

    bool rigActive() const { return composite_; }

private:
    CamPreviewSource* directCamera() const {
        auto* org = host_.liveOrganism(host_.videoSourceInto(sink_, 0));
        auto* vn = dynamic_cast<VideoNode*>(org);
        if (vn == nullptr || vn->numVideoInputs() != 0) return nullptr;
        return dynamic_cast<CamPreviewSource*>(org);
    }

    void attach(bool on) {
        if (auto* dst = dynamic_cast<VideoFrameSink*>(host_.liveOrganism(sink_)))
            dst->setVideoCordAttached(on);
    }

    void wantComposite(bool on) {
        if (on == composite_) return;
        composite_ = on;
        if (renderer_ != nullptr) renderer_->want(sink_, kFeedW, kFeedH, on);
    }

    VideoHost& host_;
    std::string sink_;
    VideoRenderService* renderer_ = nullptr;
    VisualPlanBuilder builder_;
    bool composite_ = false;
    const void* lastDeck_ = nullptr;
    unsigned gen_ = 0, camGen_ = 0;
    int tickerId_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoTrackerFeed)
};

}
