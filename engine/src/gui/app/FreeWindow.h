// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/LookAndFeel.h"
#include "gui/app/WindowMinimum.h"

namespace hum {

inline bool isCloseWindowKey(const juce::KeyPress& k) {
    return k.getModifiers().isCommandDown() && !k.getModifiers().isShiftDown()
        && k.getKeyCode() == 'W';
}

class FreeWindow : public juce::DocumentWindow, public juce::DragAndDropContainer {
public:
    FreeWindow(const juce::String& title, juce::Component* contentOwned)
        : juce::DocumentWindow(title, Palette::background, juce::DocumentWindow::closeButton) {
        setUsingNativeTitleBar(true);
        setContentOwned(contentOwned, true);
        setResizable(true, false);
        centreWithSize(contentOwned->getWidth(), contentOwned->getHeight());
        minW_ = juce::jmin(contentOwned->getWidth(), kFreeShrinkW);
        minH_ = juce::jmin(contentOwned->getHeight(), kFreeShrinkH);
        setWindowMinimumSize(*this, minW_, minH_);
        setAlwaysOnTop(true);
        setVisible(true);
        setWindowMinimumSize(*this, minW_, minH_);
    }

    void moved() override {
        juce::DocumentWindow::moved();
        refreshWindowMinimum(*this, minW_, minH_);
    }
    void resized() override {
        juce::DocumentWindow::resized();
        refreshWindowMinimum(*this, minW_, minH_);
    }

    std::function<void()> onClose;
    void closeButtonPressed() override { if (onClose) onClose(); }

    std::function<bool(const juce::KeyPress&)> onUnhandledKey;
    std::function<bool(bool)> onUnhandledKeyState;
    bool keyPressed(const juce::KeyPress& k) override {
        if (isCloseWindowKey(k)) { if (onClose) onClose(); return true; }
        return onUnhandledKey && onUnhandledKey(k);
    }
    bool keyStateChanged(bool isKeyDown) override {
        return onUnhandledKeyState && onUnhandledKeyState(isKeyDown);
    }

private:
    int minW_ = kWindowFloorW, minH_ = kWindowFloorH;
};

}
