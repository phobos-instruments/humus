// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamSchema.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/host/BrickHost.h"
#include "io/PatchDocument.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/FineDrag.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/SetValuePopup.h"

namespace hum {

class NumberFieldBrick : public PolledBrick {
public:
    NumberFieldBrick(BrickHost& host, std::string cn, std::string param)
        : PolledBrick(host, std::move(cn), 4), param_(std::move(param)) {
        if (const auto* cm = host_.model().byName(name_))
            for (const auto& d : schemaFor(cm->classRaw))
                if (d.name == param_) { lo_ = d.min; hi_ = d.max; int_ = d.isInt || d.isEnum; break; }
        last_ = value();
    }

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
        g.drawText(text(value()), getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, param_, e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        dragFrom_ = value();
        dragging_ = false;
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) return;
        if (!dragging_ && std::abs(e.getDistanceFromDragStartY()) > 2) {
            dragging_ = true;
            host_.beginParamDrag(name_, param_);
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
        if (!dragging_) return;
        double perPx = (hi_ - lo_) / 200.0;
        const bool fine = e.mods.isShiftDown();
        if (fine) perPx /= fineDragFactor();
        if (int_) perPx = std::max(perPx, fine ? 0.5 / fineDragFactor() : 0.25);
        set(dragFrom_ - e.getDistanceFromDragStartY() * perPx);
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (dragging_) { dragging_ = false; host_.endParamDrag(); }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        SetValuePopup::show(getScreenBounds(), juce::String::fromUTF8(param_.c_str()),
                            value(), lo_, hi_, int_ ? 1.0 : 0.0, Unit::None, false,
                            [safe = juce::Component::SafePointer<NumberFieldBrick>(this)](double v) {
                                if (safe != nullptr) safe->set(v);
                            });
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override {
        const double step = int_ ? 1.0 : (hi_ - lo_) / 100.0;
        set(value() + (w.deltaY > 0 ? step : -step));
    }

private:
    double value() const { return host_.liveParamValue(name_, param_); }
    juce::String text(double v) const {
        return int_ ? juce::String((int) std::llround(v)) : juce::String(v, 2);
    }
    void set(double v) {
        v = juce::jlimit(lo_, hi_, int_ ? std::round(v) : v);
        if (v != value()) { host_.editParam(name_, param_, v); repaint(); }
    }

    void poll() override {
        const double v = value();
        if (v != last_) { last_ = v; repaint(); }
    }

    std::string param_;
    double lo_ = 0.0, hi_ = 1.0, last_ = 0.0, dragFrom_ = 0.0;
    bool int_ = true, hover_ = false, dragging_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberFieldBrick)
};

}
