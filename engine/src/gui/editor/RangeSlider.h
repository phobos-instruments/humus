// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/host/BrickHost.h"
#include "gui/editor/SetValuePopup.h"
#include "gui/editor/FineDrag.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/ParamReset.h"
#include "gui/editor/inputs/RangeInput.h"

namespace hum {

class RangeSlider : public juce::Component {
public:
    using DragTarget = input::RangeInput::Target;

    RangeSlider(BrickHost& host, std::string organism, std::string param,
                double min, double max, int decimals = 1, bool logarithmic = false)
        : host_(host), name_(std::move(organism)), range_(host, name_, std::move(param), min, max, logarithmic),
          decimals_(decimals) {
        setOpaque(false);
    }

    std::function<void()> onAutomationChanged;

    void setExternallyControlled(bool b) {
        if (controlled_ == b) return;
        controlled_ = b;
        repaint();
    }

    void paintOverChildren(juce::Graphics& g) override {
        if (!controlled_) return;
        g.setColour(Palette::accent.withAlpha(alpha::mid));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.2f);
    }

    void refresh() {
        if (range_.refresh()) repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const float cx = r.getCentreX();
        const float top = trackTop();
        const float bottom = trackBottom();

        const float yLow  = range_.yOf(range_.low(), top, bottom);
        const float yHigh = range_.yOf(range_.high(), top, bottom);

        paintFaderGroove(g, r.withTop(top).withBottom(bottom), yHigh, yLow, false);

        const float halfH = 6.5f;
        const float w = juce::jlimit(12.0f, 30.0f, r.getWidth() - 6.0f);
        const auto dragging_ = range_.target();
        const bool both = dragging_ == DragTarget::Both;
        juce::Rectangle<float> topHalf(cx - w * 0.5f, yHigh - halfH, w, halfH);
        juce::Rectangle<float> botHalf(cx - w * 0.5f, yLow, w, halfH);
        drawFaderPot(g, topHalf, yHigh - halfH, yHigh + halfH, yHigh,
                     both || dragging_ == DragTarget::High, true, false, Palette::text);
        drawFaderPot(g, botHalf, yLow - halfH, yLow + halfH, yLow,
                     both || dragging_ == DragTarget::Low, false, true, Palette::text);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        refresh();
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, range_.param(), e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        range_.press((float) e.y, trackTop(), trackBottom(), e.mods.isShiftDown());
        lastY_ = (float) e.y;
        repaint();
        showBubble(0);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (range_.target() == DragTarget::None) return;
        if (e.mods.isShiftDown()) range_.dragFineBy(lastY_ - (float) e.y, trackTop(), trackBottom(), fineDragFactor());
        else range_.dragAt((float) e.y, trackTop(), trackBottom());
        lastY_ = (float) e.y;
        repaint();
        showBubble(0);
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (range_.release()) {
            bubble_.setVisible(false);
            repaint();
        }
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        SetValuePopup::showRange(
            getScreenBounds(), juce::String(range_.param()), range_.low(), range_.high(), range_.minimum(),
            range_.maximum(),
            [safe = juce::Component::SafePointer<RangeSlider>(this)](double lo, double hi) {
                if (safe == nullptr) return;
                safe->range_.setBoth(lo, hi);
                safe->repaint();
            });
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        double delta = (w.deltaY != 0.0 ? w.deltaY : w.deltaX);
        if (w.isReversed) delta = -delta;
        if (delta == 0.0) return;
        range_.wheel(delta, e.mods.isCtrlDown() || e.mods.isCommandDown() || e.mods.isShiftDown());
        repaint();
        showBubble(900);
    }

private:
    void showBubble(int timeoutMs) {
        auto* parent = getParentComponent();
        if (!parent) return;
        if (bubble_.getParentComponent() != parent) parent->addChildComponent(bubble_);
        juce::AttributedString msg(juce::String(range_.low(), decimals_) + juce::String::fromUTF8("  \xe2\x80\x93  ") +
                                   juce::String(range_.high(), decimals_));
        msg.setColour(Palette::text);
        msg.setJustification(juce::Justification::centred);
        msg.setFont(juce::Font(juce::FontOptions(12.0f)));
        const float top = trackTop(), bottom = trackBottom();
        const int yMid = (int) ((range_.yOf(range_.low(), top, bottom) + range_.yOf(range_.high(), top, bottom)) * 0.5f);
        auto a = parent->getLocalPoint(this, juce::Point<int>(getWidth() / 2, yMid));
        bubble_.showAt(juce::Rectangle<int>(a.x, a.y, 1, 1), msg, timeoutMs, false, false);
    }

    float trackTop() const { return (float) getLocalBounds().getY() + 9.0f; }
    float trackBottom() const { return (float) getLocalBounds().getBottom() - 9.0f; }

    BrickHost& host_;
    std::string name_;
    input::RangeInput range_;
    int decimals_ = 1;
    bool controlled_ = false;
    float lastY_ = 0.0f;
    juce::BubbleMessageComponent bubble_;
};

}
