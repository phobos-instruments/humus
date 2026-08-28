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
