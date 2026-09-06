#pragma once
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "gui/EngineHost.h"
#include "gui/UiTicker.h"
#include "gui/VideoPreviewStore.h"
#include "gui/VideoTakeSink.h"
#include "gui/VisualGlCanvas.h"
#include "gui/VisualPlanBuilder.h"

namespace hum {

class VideoRenderService {
public:
    static constexpr int kBrickW = 320, kBrickH = 180;

    explicit VideoRenderService(EngineHost& host) : host_(host), builder_(host) {
        tickerId_ = UiTicker::instance().add([this] { tick(); });
    }

    ~VideoRenderService() {
        UiTicker::instance().remove(tickerId_);
        stage_.reset();
    }

    void setSuppressed(std::function<bool(const std::string&)> fn) {
        suppressed_ = std::move(fn);
    }

    void want(const std::string& node, int w, int h, bool on) {
        auto& e = extra_[node];
        e.count += on ? 1 : -1;
        e.w = std::max(e.w, w);
        e.h = std::max(e.h, h);
        if (e.count <= 0) extra_.erase(node);
    }

    static bool previewable(EngineHost& host, const std::string& node) {
        auto* c = host.liveOrganism(node);
        if (c == nullptr || c->params.get("Preview", 1.0) < 0.5) return false;
        return isVideoOutputNode(host, node) || dynamic_cast<VideoNode*>(c) != nullptr;
    }

    void tick() {
        std::vector<VisualPlanBuilder::Want> wants;
        for (const auto& n : VideoPreviewStore::instance().wantedNodes()) {
            if (suppressed_ && suppressed_(n)) continue;
            if (!previewable(host_, n)) continue;
            wants.push_back({n, isVideoOutputNode(host_, n), kBrickW, kBrickH, true});
        }
        for (const auto& [node, e] : extra_)
            wants.push_back({node, true, e.w, e.h, false});
        for (const auto& e : VideoTakeStore::instance().entries())
            wants.push_back({e.node, true, e.w, e.h, false, e.sink});
        lastTapCount_ = (int) wants.size();
        if (wants.empty()) {
            stage_.reset();
            return;
        }
        int w = 2, h = 2;
        for (const auto& want : wants) {
            w = std::max(w, want.w);
            h = std::max(h, want.h);
        }
        if (stage_ == nullptr || stage_->canvas.getWidth() != w
            || stage_->canvas.getHeight() != h)
            stage_ = std::make_unique<Stage>(w, h);
        last_ = builder_.buildAll(wants);
        stage_->canvas.setPlan(last_);
        stage_->canvas.pump();
    }

    const visual::Plan& lastPlanForTest() const { return last_; }
    int lastTapCount() const { return lastTapCount_; }
    bool stageLit() const { return stage_ != nullptr; }

private:
    struct Stage : juce::Component {
        GlCanvas canvas;

        Stage(int w, int h) {
            setOpaque(true);
            canvas.setBounds(0, 0, w, h);
            addAndMakeVisible(canvas);
            const auto* display =
                juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
            const auto area = display != nullptr
                                  ? display->totalArea
                                  : juce::Rectangle<int>(0, 0, 800, 600);
            setBounds(area.getRight() - 2, area.getBottom() - 2, 2, 2);
            addToDesktop(juce::ComponentPeer::windowIsTemporary
                         | juce::ComponentPeer::windowIgnoresMouseClicks);
            setVisible(true);
            canvas.setPaused(true);
        }

        ~Stage() override { removeFromDesktop(); }

        void paint(juce::Graphics&) override {}
    };

    struct Extra {
        int count = 0, w = 0, h = 0;
    };

    EngineHost& host_;
    VisualPlanBuilder builder_;
    std::unique_ptr<Stage> stage_;
    visual::Plan last_;
    std::map<std::string, Extra> extra_;
    std::function<bool(const std::string&)> suppressed_;
    int tickerId_ = 0, lastTapCount_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoRenderService)
};

}
