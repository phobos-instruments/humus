// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

template <typename ButtonBase>
class Mappable : public ButtonBase {
public:
    using ButtonBase::ButtonBase;

    std::function<void(juce::Point<int>)> onRightClick;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            this->grabKeyboardFocus();
            if (onRightClick) onRightClick(e.getScreenPosition());
            return;
        }
        ButtonBase::mouseDown(e);
    }
};

}
