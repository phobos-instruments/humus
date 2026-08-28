#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

namespace hum {

class SizeGrip : public juce::Component {
public:
    enum Kind { Right, Bottom, Corner };

    explicit SizeGrip(Kind k) : kind_(k) {
        setMouseCursor(kind_ == Right  ? juce::MouseCursor::LeftRightResizeCursor
                     : kind_ == Bottom ? juce::MouseCursor::UpDownResizeCursor
                                       : juce::MouseCursor::BottomRightCornerResizeCursor);
        setRepaintsOnMouseActivity(true);
    }

    std::function<void(int dw, int dh, bool done)> onGesture;

    void paint(juce::Graphics& g) override {
        if (kind_ != Corner) return;
        g.setColour(isMouseOverOrDragging() ? Palette::accent : Palette::textDim);
        const float r = 1.2f;
        const float x1 = (float) getWidth() - 4.0f, y1 = (float) getHeight() - 4.0f;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j + i < 3; ++j)
                g.fillEllipse(x1 - 4.0f * (float) i, y1 - 4.0f * (float) j, r * 2, r * 2);
    }

    void mouseDown(const juce::MouseEvent&) override { dragging_ = true; }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!dragging_ || !onGesture) return;
        const auto d = e.getScreenPosition() - e.getMouseDownScreenPosition();
        onGesture(kind_ == Bottom ? 0 : d.x, kind_ == Right ? 0 : d.y, false);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (!dragging_ || !onGesture) return;
        dragging_ = false;
        const auto d = e.getScreenPosition() - e.getMouseDownScreenPosition();
        onGesture(kind_ == Bottom ? 0 : d.x, kind_ == Right ? 0 : d.y, true);
    }

private:
    Kind kind_;
    bool dragging_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SizeGrip)
};

}
