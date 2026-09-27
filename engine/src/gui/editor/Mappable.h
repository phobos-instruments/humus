// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/ParamMarks.h"

namespace hum {

template <typename ButtonBase>
class Mappable : public ButtonBase {
public:
    using ButtonBase::ButtonBase;

    std::function<void(juce::Point<int>)> onRightClick;
    std::function<void()> onRestyle;

    void setLit(bool firing) {
        if (lit_ == firing) return;
        lit_ = firing;
        this->repaint();
    }

    void setMarks(bool externallyControlled, bool rollLocked) {
        if (controlled_ == externallyControlled && locked_ == rollLocked) return;
        controlled_ = externallyControlled;
        locked_ = rollLocked;
        this->repaint();
    }

    bool markedForTest() const { return controlled_; }
    bool litForTest() const { return lit_; }

    void paintOverChildren(juce::Graphics& g) override {
        if (lit_) {
            g.setColour(Palette::accent.withAlpha(alpha::muted));
            g.fillRoundedRectangle(this->getLocalBounds().toFloat().reduced(1.0f), 4.0f);
        }
        paintParamMarks(g, this->getLocalBounds().toFloat(), controlled_, locked_);
    }

    void lookAndFeelChanged() override {
        ButtonBase::lookAndFeelChanged();
        if (onRestyle) onRestyle();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            this->grabKeyboardFocus();
            if (onRightClick) onRightClick(e.getScreenPosition());
            return;
        }
        ButtonBase::mouseDown(e);
    }

private:
    bool controlled_ = false, locked_ = false, lit_ = false;
};

}
