// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/inputs/NumberInputs.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/FineDrag.h"
#include "gui/editor/SetValuePopup.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class NumberBoxBrick : public PolledBrick {
public:
    NumberBoxBrick(BrickHost& host, std::string cn, std::string param, int decimals, double step,
                   std::string shows = {})
        : PolledBrick(host, cn, 4), box_(host, cn, std::move(param), decimals, step, std::move(shows)) {}

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 96; }
    int preferredContentHeight(int) const override { return 22; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(hover_ ? Palette::background.brighter(0.08f) : Palette::background);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f));
        g.drawText(juce::String(box_.text(box_.number().shown())), getLocalBounds().reduced(6, 0),
                   juce::Justification::centredLeft, true);
    }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, box_.number().param(), e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        dragFrom_ = box_.number().setting();
        dragging_ = false;
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) return;
        if (!dragging_ && std::abs(e.getDistanceFromDragStartY()) > 2) {
            dragging_ = true;
            box_.number().beginDrag();
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
        if (!dragging_) return;
        set(dragFrom_ - e.getDistanceFromDragStartY() * box_.step(e.mods.isShiftDown(), fineDragFactor()));
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (dragging_) { dragging_ = false; box_.number().endDrag(); }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        SetValuePopup::show(getScreenBounds(), juce::String::fromUTF8(box_.number().param().c_str()),
                            box_.number().setting(), box_.number().range().lo, box_.number().range().hi,
                            box_.number().range().integer ? 1.0 : 0.0, Unit::None, false,
                            [safe = juce::Component::SafePointer<NumberBoxBrick>(this)](double v) {
                                if (safe != nullptr) safe->set(v);
                            });
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        const double per = box_.step(e.mods.isShiftDown(), fineDragFactor());
        set(box_.number().setting() + (w.deltaY > 0 ? per : -per));
    }

private:
    void set(double v) {
        if (box_.number().set(v)) repaint();
    }

    void poll() override {
        if (box_.number().poll()) repaint();
    }

    input::NumberBox box_;
    double dragFrom_ = 0.0;
    bool hover_ = false, dragging_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberBoxBrick)
};

}
