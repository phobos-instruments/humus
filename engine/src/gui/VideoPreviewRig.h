#pragma once
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/UiTicker.h"
#include "gui/VisualGlCanvas.h"
#include "gui/VisualPlanBuilder.h"

namespace hum {

class VideoPreviewRig : public juce::Component {
public:
    VideoPreviewRig(EngineHost& host, const std::string& node)
        : builder_(host, node,true) {
        setOpaque(true);
        canvas_ = std::make_unique<GlCanvas>();
        canvas_->setPreviewNode(node);
        canvas_->setBounds(0, 0, 160, 90);
        addAndMakeVisible(*canvas_);
        const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        const auto area = display != nullptr ? display->totalArea
                                             : juce::Rectangle<int>(0, 0, 800, 600);
        setBounds(area.getRight() - 2, area.getBottom() - 2, 2, 2);
        addToDesktop(juce::ComponentPeer::windowIsTemporary
                     | juce::ComponentPeer::windowIgnoresMouseClicks);
        setVisible(true);
        canvas_->setPaused(true);
        tickerId_ = UiTicker::instance().add([this] {
            canvas_->setPlan(builder_.build());
            canvas_->pump();
        });
    }

    ~VideoPreviewRig() override {
        UiTicker::instance().remove(tickerId_);
        canvas_.reset();
        removeFromDesktop();
    }

private:
    VisualPlanBuilder builder_;
    std::unique_ptr<GlCanvas> canvas_;
    int tickerId_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoPreviewRig)
};

}
