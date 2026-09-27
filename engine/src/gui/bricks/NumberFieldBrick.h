// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/juce/PanelScroller.h"

#include "gui/editor/inputs/NumberInputs.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/FineDrag.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/SetValuePopup.h"

namespace hum {

class NumberFieldBrick : public PolledBrick {
public:
    NumberFieldBrick(BrickHost& host, std::string cn, std::string param)
        : PolledBrick(host, cn, 4), number_(host, cn, std::move(param)) {}

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 64; }
    int preferredContentHeight(int) const override { return 20; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(hover_ ? Palette::background.brighter(0.08f) : Palette::background);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(juce::String(number_.fieldText(number_.setting())), getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, number_.param(), e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        dragFrom_ = number_.setting();
        dragging_ = false;
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) return;
        if (!dragging_ && std::abs(e.getDistanceFromDragStartY()) > 2) {
            dragging_ = true;
            number_.beginDrag();
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
        if (!dragging_) return;
        const double perPx = number_.fieldDragPerPixel(e.mods.isShiftDown(), fineDragFactor());
        set(dragFrom_ - e.getDistanceFromDragStartY() * perPx);
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (dragging_) { dragging_ = false; number_.endDrag(); }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        SetValuePopup::show(getScreenBounds(), juce::String::fromUTF8(number_.param().c_str()),
                            number_.setting(), number_.range().lo, number_.range().hi,
                            number_.range().integer ? 1.0 : 0.0, Unit::None, false,
                            [safe = juce::Component::SafePointer<NumberFieldBrick>(this)](double v) {
                                if (safe != nullptr) safe->set(v);
                            });
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        if (PanelScroller::takesWheel(*this, e, w)) return;
        const double step = number_.fieldWheelStep();
        set(number_.setting() + (w.deltaY > 0 ? step : -step));
    }

private:
    void set(double v) {
        if (number_.set(v)) repaint();
    }

    void poll() override {
        if (number_.poll()) repaint();
    }

    input::NumberInput number_;
    double dragFrom_ = 0.0;
    bool hover_ = false, dragging_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberFieldBrick)
};

}
