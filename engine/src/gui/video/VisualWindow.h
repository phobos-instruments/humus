// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/FreeWindow.h"
#include "gui/video/FpsMeter.h"
#include "gui/video/VisualGlCanvas.h"
#include "gui/video/VisualPlanBuilder.h"

#if JUCE_MAC
namespace hum {
void windowKeepFullscreenLocal(juce::ComponentPeer* peer);
}
#endif

namespace hum {

class VisualWindow : public juce::DocumentWindow, private juce::Timer {
public:
    static constexpr int kCanvasMinW = 320;
    static constexpr int kCanvasMinH = 180;

    VisualWindow(VideoHost& host, std::string name, juce::Component* mainComponent,
                 std::function<void(const std::string&)> onClosed)
        : juce::DocumentWindow(juce::String(name), juce::Colours::black,
                               juce::DocumentWindow::allButtons),
          host_(host), name_(std::move(name)),
          onClosed_(std::move(onClosed)),
          isOutput_(isVideoOutputNode(host_, name_)),
          builder_(host_, name_, isOutput_) {
        juce::ignoreUnused(mainComponent);
        setUsingNativeTitleBar(true);
        canvas_ = std::make_unique<GlCanvas>();
        if (isOutput_) canvas_->setPreviewNode(name_);
        canvas_->setSize(640, 360);
        setContentNonOwned(canvas_.get(), true);
        setResizable(true, false);
        setWindowMinimumSize(*this, kCanvasMinW, kCanvasMinH);
        setVisible(true);
        setWindowMinimumSize(*this, kCanvasMinW, kCanvasMinH);
        startTimerHz(30);
#if JUCE_MAC
        windowKeepFullscreenLocal(getPeer());
#endif
    }

    ~VisualWindow() override {
        stopTimer();
        canvas_.reset();
#if JUCE_MAC
        removeFromDesktop();
#endif
        clearContentComponent();
    }

    void closeButtonPressed() override { if (onClosed_) onClosed_(name_); }

    void moved() override {
        juce::DocumentWindow::moved();
        if (isResizable()) refreshWindowMinimum(*this, kCanvasMinW, kCanvasMinH);
    }
    void resized() override {
        juce::DocumentWindow::resized();
        if (isResizable()) refreshWindowMinimum(*this, kCanvasMinW, kCanvasMinH);
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (isCloseWindowKey(k)) { closeButtonPressed(); return true; }
        return juce::DocumentWindow::keyPressed(k);
    }

private:
    void applyScreen(int screen) {
        if (screen == lastScreen_) return;
        lastScreen_ = screen;
        const auto& ds = juce::Desktop::getInstance().getDisplays().displays;
        if (screen >= 1 && screen <= ds.size()) {
            setUsingNativeTitleBar(false);
            setTitleBarHeight(0);
            setResizable(false, false);
            setAlwaysOnTop(true);
            setBounds(ds.getReference(screen - 1).totalArea);
            toFront(false);
        } else {
            setAlwaysOnTop(false);
            setUsingNativeTitleBar(true);
            setResizable(true, false);
            centreWithSize(656, 396);
            setWindowMinimumSize(*this, kCanvasMinW, kCanvasMinH);
        }
#if JUCE_MAC
        windowKeepFullscreenLocal(getPeer());
#endif
    }

    void timerCallback() override {
        if (host_.model().byName(name_) == nullptr) {
            if (onClosed_) onClosed_(name_);
            return;
        }
        canvas_->setPaused(isMinimised());
        canvas_->setPlan(builder_.build());
        if (isOutput_) applyScreen((int) builder_.paramOr(name_, "Screen", 0.0f));

        juce::String err = builder_.parseError();
        if (err.isEmpty()) err = canvas_->compileError();
        juce::String title = juce::String(name_);
        if (isOutput_ && builder_.root().empty())
            title += " " + juce::String("-") + " no input corded";
        meter_.note(canvas_->renderCount());
        if (fpsOverlayOn() && meter_.fps() > 0.0)
            title += " " + juce::String("-") + " " + meter_.label();
        setName(err.isEmpty() ? title
                              : juce::String(name_) + " " + juce::String("-")
                                    + " scene error: " + err.upToFirstOccurrenceOf("\n", false, false));
    }

    VideoHost& host_;
    std::string name_;
    std::function<void(const std::string&)> onClosed_;
    const bool isOutput_;
    VisualPlanBuilder builder_;
    std::unique_ptr<GlCanvas> canvas_;
    FpsMeter meter_;
    int lastScreen_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualWindow)
};

}
