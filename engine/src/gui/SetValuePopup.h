#pragma once
#include <cmath>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamUnit.h"
#include "gui/LookAndFeel.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class SetValuePopup : public juce::Component {
public:
    static juce::String pitchName(double hz) {
        if (hz < 8.0 || hz > 20000.0) return {};
        const double n = hzToMidi(hz);
        const int note = (int) std::lround(n);
        if (note < 0 || note > 135) return {};
        static const char* kNames[12] = {"C",  "C#", "D",  "D#", "E",  "F",
                                         "F#", "G",  "G#", "A",  "A#", "B"};
        const int cents = (int) std::lround((n - note) * 100.0);
        juce::String s = juce::String(kNames[note % 12]) + juce::String(note / 12 - 1);
        if (cents != 0) s << " " << (cents > 0 ? "+" : "") << cents << "c";
        return s;
    }

    static juce::String textFor(Unit u, double v, double min, double max, double interval) {
        if (u != Unit::None) return unitPlain(u, v, min, max);
        return interval > 0.0 && std::abs(interval - std::round(interval)) < 1e-9
                   ? juce::String((int) std::lround(v))
                   : juce::String(v, 3);
    }

    static double valueFor(Unit u, const juce::String& text, double min, double max) {
        return juce::jlimit(min, max, u != Unit::None ? unitPlainParse(u, text, min, max)
                                                      : text.getDoubleValue());
    }

    static void show(juce::Rectangle<int> screenArea, const juce::String& title,
                     double value, double min, double max, double interval, Unit unit,
                     bool showPitch, std::function<void(double)> onApply) {
        auto content = std::make_unique<SetValuePopup>(title, min, max, interval, showPitch,
                                                       std::move(onApply), false, value, 0.0,
                                                       unit);
        juce::CallOutBox::launchAsynchronously(std::move(content), screenArea, nullptr);
    }

    static void showRange(juce::Rectangle<int> screenArea, const juce::String& title,
                          double lo, double hi, double min, double max,
                          std::function<void(double, double)> onApplyRange) {
        auto content = std::make_unique<SetValuePopup>(title, min, max, 0.0, false,
                                                       nullptr, true, lo, hi, Unit::None);
        content->onApplyRange_ = std::move(onApplyRange);
        juce::CallOutBox::launchAsynchronously(std::move(content), screenArea, nullptr);
    }

    SetValuePopup(const juce::String& title, double min, double max, double interval,
                  bool showPitch, std::function<void(double)> onApply,
                  bool isRange, double v1, double v2, Unit unit)
        : min_(min), max_(max), interval_(interval), unit_(unit), showPitch_(showPitch),
          isRange_(isRange), onApply_(std::move(onApply)) {
        title_.setText(title, juce::dontSendNotification);
        title_.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
        addAndMakeVisible(title_);

        auto initBox = [this](ValueBox& ed, double v) {
            ed.setJustification(juce::Justification::centredRight);
            ed.setInputRestrictions(12, "0123456789.-eE");
            ed.setText(format(v), juce::dontSendNotification);
            ed.setSelectAllWhenFocused(true);
            ed.onReturnKey = [this] { apply(); };
            ed.onEscapeKey = [this] { dismiss(); };
            ed.onTextChange = [this] { refreshPitch(); };
            ed.onNudge = [this, &ed](int dir) { nudge(ed, dir); };
            addAndMakeVisible(ed);
        };
        initBox(box1_, v1);
        if (isRange_) initBox(box2_, v2);

        up_.onClick = [this] { nudge(box1_, +1); };
        down_.onClick = [this] { nudge(box1_, -1); };
        addChildComponent(up_);
        addChildComponent(down_);
        up_.setVisible(!isRange_);
        down_.setVisible(!isRange_);

        suffix_.setText(unitSuffix(unit_), juce::dontSendNotification);
        suffix_.setColour(juce::Label::textColourId, Palette::textDim);
        suffix_.setFont(juce::FontOptions(12.0f));
        addChildComponent(suffix_);
        suffix_.setVisible(suffix_.getText().isNotEmpty());

        pitch_.setColour(juce::Label::textColourId, Palette::textDim);
        pitch_.setFont(juce::FontOptions(12.0f));
        addChildComponent(pitch_);
        pitch_.setVisible(showPitch_);
        refreshPitch();

        ok_.onClick = [this] { apply(); };
        addAndMakeVisible(ok_);

        const int w = isRange_ ? 232 : (showPitch_ ? 250 : 180);
        setSize(w + (suffix_.isVisible() ? 30 : 0), 62);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8, 6);
        title_.setBounds(r.removeFromTop(16));
        r.removeFromTop(4);
        auto row = r.removeFromTop(24);
        if (isRange_) {
            box1_.setBounds(row.removeFromLeft(74));
            row.removeFromLeft(6);
            box2_.setBounds(row.removeFromLeft(74));
        } else {
            box1_.setBounds(row.removeFromLeft(82));
            if (suffix_.isVisible()) {
                row.removeFromLeft(3);
                suffix_.setBounds(row.removeFromLeft(26));
            }
            row.removeFromLeft(2);
            auto arrows = row.removeFromLeft(18);
            up_.setBounds(arrows.removeFromTop(12));
            down_.setBounds(arrows.removeFromTop(12));
            if (showPitch_) {
                row.removeFromLeft(4);
                pitch_.setBounds(row.removeFromLeft(70));
            }
        }
        ok_.setBounds(getWidth() - 58, box1_.getY(), 48, 24);
    }

    void parentHierarchyChanged() override {
        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<SetValuePopup>(this)] {
                if (safe != nullptr && safe->isShowing())
                    safe->box1_.grabKeyboardFocus();
            });
    }

private:
    struct ValueBox : juce::TextEditor {
        std::function<void(int)> onNudge;
        bool keyPressed(const juce::KeyPress& k) override {
            if (k.getKeyCode() == juce::KeyPress::upKey) {
                if (onNudge) onNudge(+1);
                return true;
            }
            if (k.getKeyCode() == juce::KeyPress::downKey) {
                if (onNudge) onNudge(-1);
                return true;
            }
            return juce::TextEditor::keyPressed(k);
        }
    };

    juce::String format(double v) const { return textFor(unit_, v, min_, max_, interval_); }

    double step() const {
        switch (unit_) {
            case Unit::Decibels: case Unit::Db:     return 0.5;
            case Unit::Percent: case Unit::Signed:  return 1.0;
            default: return interval_ > 0.0 ? interval_ : (max_ - min_) * 0.01;
        }
    }

    void nudge(juce::TextEditor& ed, int dir) {
        const auto shown = juce::String(ed.getText().getDoubleValue() + dir * step(), 6);
        ed.setText(format(valueFor(unit_, shown, min_, max_)), juce::dontSendNotification);
        ed.selectAll();
        refreshPitch();
    }

    void refreshPitch() {
        if (!showPitch_) return;
        pitch_.setText(pitchName(box1_.getText().getDoubleValue()), juce::dontSendNotification);
    }

    void apply() {
        const double v1 = valueFor(unit_, box1_.getText(), min_, max_);
        if (isRange_) {
            const double v2 = juce::jlimit(v1, max_, box2_.getText().getDoubleValue());
            if (onApplyRange_) onApplyRange_(v1, v2);
        } else if (onApply_) {
            onApply_(v1);
        }
        dismiss();
    }

    void dismiss() {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
    }

    double min_, max_, interval_;
    Unit unit_;
    bool showPitch_, isRange_;
    std::function<void(double)> onApply_;
    std::function<void(double, double)> onApplyRange_;
    juce::Label title_, pitch_, suffix_;
    ValueBox box1_, box2_;
    juce::TextButton up_{"+"}, down_{"-"}, ok_{"OK"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetValuePopup)
};

}
