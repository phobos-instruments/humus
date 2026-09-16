// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamSchema.h"
#include "gui/bricks/PolledBrick.h"
#include "hum/caps/Graph.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/FineDrag.h"
#include "gui/editor/SetValuePopup.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "io/PatchDocument.h"

namespace hum {

class NumberBoxBrick : public PolledBrick {
public:
    NumberBoxBrick(BrickHost& host, std::string cn, std::string param, int decimals, double step,
                   std::string shows = {})
        : PolledBrick(host, std::move(cn), 4), param_(std::move(param)),
          shows_(std::move(shows)), decimals_(juce::jlimit(0, 6, decimals)) {
        if (const auto* cm = host_.model().byName(name_))
            for (const auto& d : schemaFor(cm->classRaw))
                if (d.name == param_) { lo_ = d.min; hi_ = d.max; int_ = d.isInt || d.isEnum; break; }
        step_ = step > 0.0 ? step : (int_ ? 1.0 : std::pow(10.0, -decimals_));
        last_ = value();
    }

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
        g.drawText(text(value()), getLocalBounds().reduced(6, 0),
                   juce::Justification::centredLeft, true);
    }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, param_, e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        dragFrom_ = setting();
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
        const double per = e.mods.isShiftDown() ? step_ / fineDragFactor() : step_;
        set(dragFrom_ - e.getDistanceFromDragStartY() * per);
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (dragging_) { dragging_ = false; host_.endParamDrag(); }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        SetValuePopup::show(getScreenBounds(), juce::String::fromUTF8(param_.c_str()),
                            setting(), lo_, hi_, int_ ? 1.0 : 0.0, Unit::None, false,
                            [safe = juce::Component::SafePointer<NumberBoxBrick>(this)](double v) {
                                if (safe != nullptr) safe->set(v);
                            });
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        const double per = e.mods.isShiftDown() ? step_ / fineDragFactor() : step_;
        set(setting() + (w.deltaY > 0 ? per : -per));
    }

private:
    double value() const {
        if (!shows_.empty())
            if (const auto* src = dynamic_cast<const ControlSource*>(host_.liveOrganism(name_))) {
                ControlSource::ControlVal vals[16];
                const int n = src->controlValues(vals, 16);
                for (int i = 0; i < n; ++i)
                    if (shows_ == vals[i].name) return vals[i].value;
            }
        return host_.liveParamValue(name_, param_);
    }

    double setting() const { return host_.liveParamValue(name_, param_); }

    juce::String text(double v) const {
        return int_ || decimals_ == 0 ? juce::String((juce::int64) std::llround(v))
                                      : juce::String(v, decimals_);
    }

    void set(double v) {
        v = juce::jlimit(lo_, hi_, int_ ? std::round(v) : v);
        if (v != setting()) { host_.editParam(name_, param_, v); repaint(); }
    }

    void poll() override {
        const double v = value();
        if (v != last_) { last_ = v; repaint(); }
    }

    std::string param_;
    std::string shows_;
    int decimals_ = 2;
    double lo_ = 0.0, hi_ = 1.0, last_ = 0.0, dragFrom_ = 0.0, step_ = 1.0;
    bool int_ = true, hover_ = false, dragging_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberBoxBrick)
};

}
