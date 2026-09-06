#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

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
        setAlwaysOnTop(true);
        setVisible(true);
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
};

}
