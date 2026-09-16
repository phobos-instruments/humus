// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "gui/style/Colours.h"
#include "gui/app/FreeWindow.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"
#include "gui/app/Dock.h"

namespace hum {

class FloatingBoxWindow : public juce::DocumentWindow, public juce::DragAndDropContainer {
public:
    FloatingBoxWindow(const juce::String& name, juce::Component* content, juce::Rectangle<int> bounds,
                      std::function<void()> onCloseRequested,
                      std::function<void(juce::Rectangle<int>)> onBoundsChanged)
        : juce::DocumentWindow(name, Palette::background, juce::DocumentWindow::closeButton),
          onClose_(std::move(onCloseRequested)), onBounds_(std::move(onBoundsChanged)) {
        setUsingNativeTitleBar(true);
        holder_.take(content);
        holder_.setSize(content->getWidth() + 2 * BoxHolder::kInset, content->getHeight() + 2 * BoxHolder::kInset);
        natural_ = {holder_.getWidth(), holder_.getHeight()};
        setContentNonOwned(&holder_, true);
        setResizable(true, false);
        const auto& displays = juce::Desktop::getInstance().getDisplays();
        const bool sane = bounds.getWidth() >= kMinSane && bounds.getHeight() >= kMinSane
                          && displays.getDisplayForRect(bounds) != nullptr
                          && displays.getDisplayForRect(bounds)->userArea.intersects(bounds);
        if (sane) setBounds(bounds);
        else if (const auto* d = displays.getPrimaryDisplay(); d != nullptr)
            setBounds(d->userArea.withSizeKeepingCentre(getWidth(), getHeight()));
        else centreWithSize(getWidth(), getHeight());
        applyMinimum();
        target_ = getBounds();
        setAlwaysOnTop(true);
        setVisible(true);
        applyMinimum();
        shown_ = true;
        shownAt_ = juce::Time::getMillisecondCounter();
        if (std::getenv("HUM_FLOAT_DEBUG") != nullptr)
            std::fprintf(stderr, "[float] %s: stored %s (%s), content %dx%d, window %s\n",
                         name.toRawUTF8(), bounds.toString().toRawUTF8(), sane ? "used" : "ignored",
                         content->getWidth(), content->getHeight(), getBounds().toString().toRawUTF8());
    }

    static constexpr int kMinSane = 160;
    static constexpr juce::uint32 kSettleMs = 400;
    static constexpr int kFrameSlack = 64;
    bool settling() const { return juce::Time::getMillisecondCounter() - shownAt_ < kSettleMs; }
    bool reportable() const { return shown_ && isVisible() && !settling() && getWidth() >= kMinSane && getHeight() >= kMinSane; }
    void releaseContent() { shown_ = false; holder_.release(); clearContentComponent(); }
    void fitContent() {
        if (holder_.box == nullptr) return;
        const juce::ScopedValueSetter<bool> guard(fitting_, true);
        natural_ = {holder_.box->getWidth() + 2 * BoxHolder::kInset,
                    holder_.box->getHeight() + 2 * BoxHolder::kInset};
        holder_.setSize(juce::jmax(holder_.getWidth(), natural_.x),
                        juce::jmax(holder_.getHeight(), natural_.y));
        holder_.resized();
        applyMinimum();
        target_ = getBounds();
    }
    void closeButtonPressed() override { if (onClose_) onClose_(); }
    void moved() override { juce::DocumentWindow::moved(); report("moved"); }
    void resized() override { juce::DocumentWindow::resized(); report("resized"); }
    void report(const char* why) {
        syncMinimum();
        refreshWindowMinimum(*this, natural_.x, natural_.y);
        if (std::getenv("HUM_FLOAT_DEBUG") != nullptr)
            std::fprintf(stderr, "[float] %s: %s to %s%s\n", getName().toRawUTF8(), why,
                         getBounds().toString().toRawUTF8(), reportable() ? "" : " (not recorded)");
        if (shown_ && settling() && !asserting_ && !fitting_) {
            const bool sizeOff = getWidth() != target_.getWidth() || getHeight() != target_.getHeight();
            const bool farOff = std::abs(getX() - target_.getX()) > kFrameSlack
                                || std::abs(getY() - target_.getY()) > kFrameSlack;
            if (sizeOff || farOff) {
                asserting_ = true;
                setBounds(farOff ? target_ : getBounds().withSize(target_.getWidth(), target_.getHeight()));
                asserting_ = false;
                return;
            }
        }
        if (reportable() && onBounds_) { target_ = getBounds(); onBounds_(getBounds()); }
    }

    std::function<bool(const juce::KeyPress&)> onUnhandledKey;
    std::function<bool(bool)> onUnhandledKeyState;
    bool keyPressed(const juce::KeyPress& k) override {
        if (isCloseWindowKey(k)) { if (onClose_) onClose_(); return true; }
        return onUnhandledKey && onUnhandledKey(k);
    }
    bool keyStateChanged(bool isKeyDown) override {
        return onUnhandledKeyState && onUnhandledKeyState(isKeyDown);
    }

private:
    void applyMinimum() { setWindowMinimumSize(*this, natural_.x, natural_.y); }

    void syncMinimum() {
        const auto frame = windowFrameSize(*this);
        if (framed_ && frame == frame_) return;
        frame_ = frame;
        framed_ = true;
        const auto was = getBounds();
        const juce::ScopedValueSetter<bool> guard(fitting_, true);
        applyMinimum();
        if (getBounds() != was) target_ = getBounds();
    }

    struct BoxHolder : juce::Component {
        static constexpr int kInset = 6;
        juce::Component* box = nullptr;
        void take(juce::Component* c) { box = c; addAndMakeVisible(c); }
        void release() { if (box != nullptr) removeChildComponent(box); box = nullptr; }
        void resized() override { if (box != nullptr) box->setBounds(getLocalBounds().reduced(kInset)); }
        void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }
    };
    BoxHolder holder_;
    std::function<void()> onClose_;
    std::function<void(juce::Rectangle<int>)> onBounds_;
    juce::Rectangle<int> target_;
    juce::uint32 shownAt_ = 0;
    juce::Point<int> natural_;
    juce::BorderSize<int> frame_;
    bool shown_ = false, asserting_ = false, fitting_ = false, framed_ = false;
};

class FloatingPaneWindow : public juce::DocumentWindow, public juce::DragAndDropContainer {
public:
    static constexpr int kPaneMinW = 320;
    static constexpr int kPaneMinH = 220;

    FloatingPaneWindow(const juce::String& name, juce::Component* content,
                       juce::Rectangle<int> bounds, std::function<void()> onCloseRequested,
                       juce::Component* strip = nullptr, int stripH = 0)
        : juce::DocumentWindow(name, Palette::background, juce::DocumentWindow::closeButton),
          onClose_(std::move(onCloseRequested)) {
        setUsingNativeTitleBar(true);
        holder_.setParts(strip, stripH, content);
        setContentNonOwned(&holder_, false);
        setResizable(true, false);
        if (bounds.getWidth() > 50 && bounds.getHeight() > 50) setBounds(bounds);
        else                                                   centreWithSize(360, 420);
        setWindowMinimumSize(*this, kPaneMinW, kPaneMinH);
        setVisible(true);
        setWindowMinimumSize(*this, kPaneMinW, kPaneMinH);
    }

    void moved() override {
        juce::DocumentWindow::moved();
        refreshWindowMinimum(*this, kPaneMinW, kPaneMinH);
    }
    void resized() override {
        juce::DocumentWindow::resized();
        refreshWindowMinimum(*this, kPaneMinW, kPaneMinH);
    }

    void releaseContent() { holder_.release(); }
    void closeButtonPressed() override { if (onClose_) onClose_(); }

    std::function<bool(const juce::KeyPress&)> onUnhandledKey;
    std::function<bool(bool)> onUnhandledKeyState;
    bool keyPressed(const juce::KeyPress& k) override {
        if (isCloseWindowKey(k)) { if (onClose_) onClose_(); return true; }
        return onUnhandledKey && onUnhandledKey(k);
    }
    bool keyStateChanged(bool isKeyDown) override {
        return onUnhandledKeyState && onUnhandledKeyState(isKeyDown);
    }

private:
    PaneHolder holder_;
    std::function<void()> onClose_;
};

}
