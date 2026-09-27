// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/net/ControlShape.h"
#include "gui/editor/DragNumberEditor.h"
#include "gui/properties/BehaviourPictures.h"
#include "gui/properties/ControlKindButton.h"
#include "gui/properties/MappingCurveEditor.h"
#include "gui/properties/ModeChips.h"

namespace hum {

struct InspectedSource {
    juce::String title;
    ControlFamily family = ControlFamily::Midi;
    ControlShape shape;
    std::vector<int> held;
    juce::String message, number, channel, device, origin;
    bool numbered = false;
    bool bounded = false;
    juce::String minText, maxText, unit;
    CurveAxes axes;
};

class MappingInspector : public juce::Component {
public:
    std::function<void(const ControlShape&)> onChanged;

    juce::TextButton message, channel, device;
    DragNumberEditor number, min, max;

    MappingInspector();

    void show(const InspectedSource& source);
    void clear(const juce::String& why);
    void setLive(const juce::String& readout, double input01, double waitingAt01);

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Rectangle<int> fromWordBounds() const { return fromWord_.getBounds(); }
    juce::Rectangle<int> toWordBounds() const { return toWord_.getBounds(); }
    juce::Rectangle<int> unitWordBounds() const { return unitWord_.getBounds(); }
    juce::String unitWordText() const { return unitWord_.getText(); }
    juce::Button* typeChipForTest(int i) { return types_.chipForTest(i); }
    juce::String hintForTest() const { return hint_.getText(); }
    juce::String titleForTest() const { return title_.getText(); }
    bool showsCurveForTest() const { return curve_.isVisible(); }
    bool showsTimelineForTest() const { return timeline_.isVisible(); }
    bool showsStairsForTest() const { return stairs_.isVisible(); }
    int curveHeightForTest() const { return curve_.getHeight(); }
    MappingCurveEditor& curveForTest() { return curve_; }

private:
    void refresh();
    void emit() { if (onChanged) onChanged(shape_); }
    void label(juce::Label& l, const juce::String& text, float size, bool dim);
    void commitOn(DragNumberEditor& ed, std::function<void()> write);
    void layoutSettings(juce::Rectangle<int> row);

    ControlShape shape_;
    ControlFamily family_ = ControlFamily::Midi;
    bool has_ = false, midi_ = false, numbered_ = false, bounded_ = false, combo_ = false;
    juce::Label title_, live_, origin_, kindLabel_, modeLabel_, hint_, empty_;
    juce::Label smoothLabel_, thresholdLabel_, stepLabel_, fromWord_, toWord_, unitWord_;
    HeldChips held_;
    ModeChips<ControlType> types_;
    ModeChips<ButtonMode> buttons_;
    ModeChips<FaderMode> faders_;
    ModeChips<EncoderFormat> encoders_;
    DragNumberEditor smooth_, threshold_, step_;
    juce::ToggleButton inverted_;
    MappingCurveEditor curve_;
    PressTimeline timeline_;
    EncoderStairs stairs_;
    static constexpr double kMaxSmoothingSeconds = 30.0;
    static constexpr double kSmoothingPerPixel = 0.01;
    static constexpr double kMinStepPercent = 0.1;
};

}
