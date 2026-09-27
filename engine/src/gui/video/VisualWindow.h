// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/FreeWindow.h"
#include "gui/video/FpsMeter.h"
#include "gui/video/VisualGlCanvas.h"
#include "gui/video/VisualPlanBuilder.h"
#include "gui/video/WindowFloat.h"

#if JUCE_MAC
namespace hum {
void windowKeepFullscreenLocal(juce::ComponentPeer* peer);
}
#endif

namespace hum {

class VisualStage : public juce::Component {
public:
    std::function<void()> onDoubleClick;

    void hold(juce::Component& picture) {
        addAndMakeVisible(picture);
        picture.setInterceptsMouseClicks(false, false);
        picture_ = &picture;
        resized();
    }

    void resized() override {
        if (picture_ != nullptr) picture_->setBounds(getLocalBounds());
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        if (onDoubleClick) onDoubleClick();
    }

private:
    juce::Component* picture_ = nullptr;
};

class VisualWindow : public juce::DocumentWindow, private juce::Timer {
public:
    static constexpr int kPlanHz = 60;
    static constexpr int kCanvasMinW = 320;
    static constexpr int kCanvasMinH = 180;
    static constexpr int kCanvasW = 1280;
    static constexpr int kCanvasH = 720;
    static constexpr int kFrameW = 16;
    static constexpr int kFrameH = 36;

    VisualWindow(VideoHost& host, std::string name, juce::Component* mainComponent,
                 std::function<void(const std::string&)> onClosed,
                 int width = 0, int height = 0)
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
        sizeCanvas(width, height);
        stage_.hold(*canvas_);
        stage_.onDoubleClick = [this] { leaveFullScreen(); };
        stage_.setSize(wantW_, wantH_);
        setContentNonOwned(&stage_, true);
        setResizable(true, false);
        setWindowMinimumSize(*this, kCanvasMinW, kCanvasMinH);
        setVisible(true);
        setWindowMinimumSize(*this, kCanvasMinW, kCanvasMinH);
        startTimerHz(kPlanHz);
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

    void openAt(int width, int height) {
        sizeCanvas(width, height);
        if (!isResizable()) return;
        centreWithSize(wantW_ + kFrameW, wantH_ + kFrameH);
        setWindowMinimumSize(*this, kCanvasMinW, kCanvasMinH);
    }

    static juce::Rectangle<int> roomOnScreen() {
        const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        return display != nullptr ? display->userArea : juce::Rectangle<int>(0, 0, 1280, 720);
    }

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
        if (k.isKeyCode(juce::KeyPress::escapeKey) && fullScreen()) {
            leaveFullScreen();
            return true;
        }
        return juce::DocumentWindow::keyPressed(k);
    }

    bool fullScreen() const {
        return isOutput_
            && windowfloat::fillsDisplay(lastScreen_,
                                         juce::Desktop::getInstance().getDisplays().displays.size());
    }

    void guardDisplay(int screen) {
        guardedScreen_ = screen;
        lastScreen_ = -1;
    }
    bool guarding() const { return guardedScreen_ > 0; }

    void leaveFullScreen() {
        if (!fullScreen()) return;
        host_.setParam(name_, "Screen", 0.0);
        host_.notePanelEdit(name_);
    }

private:
    void sizeCanvas(int width, int height) {
        const auto room = roomOnScreen();
        const int wideEnough = std::max(kCanvasMinW, room.getWidth() - kFrameW);
        const int tallEnough = std::max(kCanvasMinH, room.getHeight() - kFrameH);
        wantW_ = std::min(width > 0 ? width : kCanvasW, wideEnough);
        wantH_ = std::min(height > 0 ? height : kCanvasH, tallEnough);
    }

    void applyScreen(int wanted, bool keepOnTop) {
        const int displays = juce::Desktop::getInstance().getDisplays().displays.size();
        guardedScreen_ = windowfloat::keptGuard(guardedScreen_, wanted);
        const int screen = windowfloat::effectiveScreen(wanted, displays, guardedScreen_);
        const bool above = windowfloat::staysAbove(screen, displays, keepOnTop);
        if (screen != lastScreen_) placeOn(screen, displays);
        if (above != isAlwaysOnTop()) setAlwaysOnTop(above);
    }

    void placeOn(int screen, int displays) {
        lastScreen_ = screen;
        if (windowfloat::fillsDisplay(screen, displays)) {
            setUsingNativeTitleBar(false);
            setTitleBarHeight(0);
            setResizable(false, false);
            setBounds(juce::Desktop::getInstance().getDisplays().displays.getReference(screen - 1).totalArea);
            setWantsKeyboardFocus(true);
            toFront(true);
            grabKeyboardFocus();
        } else {
            setUsingNativeTitleBar(true);
            setResizable(true, false);
            centreWithSize(wantW_ + kFrameW, wantH_ + kFrameH);
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
        if (isOutput_)
            applyScreen((int) builder_.paramOr(name_, "Screen", 0.0f), builder_.paramOr(name_, "OnTop", 1.0f) >= 0.5f);

        juce::String err = builder_.parseError();
        if (err.isEmpty()) err = canvas_->compileError();
        juce::String title = juce::String(name_);
        if (isOutput_ && builder_.root().empty())
            title += " " + juce::String("-") + " no input corded";
        meter_.note(canvas_->renderCount());
        if (fpsOverlayOn(name_) && meter_.fps() > 0.0)
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
    VisualStage stage_;
    FpsMeter meter_;
    int lastScreen_ = -1;
    int guardedScreen_ = 0;
    int wantW_ = kCanvasW, wantH_ = kCanvasH;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualWindow)
};

}
