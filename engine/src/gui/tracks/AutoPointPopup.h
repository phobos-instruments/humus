// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamUnit.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/SetValuePopup.h"
#include "gui/tracks/TimelineGrid.h"
#include "io/MeterMap.h"

namespace hum {

class AutoPointPopup : public juce::Component {
public:
    struct Point {
        std::string kind;
        double beat = 0.0;
        double value = 0.0;
        double valueMax = 0.0;
    };

    static void show(juce::Rectangle<int> screenArea, const juce::String& title, Point point,
                     double min, double max, Unit unit, MeterMap meters,
                     std::function<void(Point)> onApply) {
        auto content = std::make_unique<AutoPointPopup>(title, point, min, max, unit,
                                                        std::move(meters), std::move(onApply));
        juce::CallOutBox::launchAsynchronously(std::move(content), screenArea, nullptr);
    }

    AutoPointPopup(const juce::String& title, Point point, double min, double max, Unit unit,
                   MeterMap meters, std::function<void(Point)> onApply)
        : point_(std::move(point)), min_(min), max_(max), unit_(unit), meters_(std::move(meters)),
          onApply_(std::move(onApply)) {
        title_.setText(title, juce::dontSendNotification);
        title_.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
        addAndMakeVisible(title_);

        auto initBox = [this](juce::TextEditor& ed, const juce::String& text, const char* allowed) {
            ed.setJustification(juce::Justification::centredRight);
            ed.setInputRestrictions(14, allowed);
            ed.setText(text, juce::dontSendNotification);
            ed.setSelectAllWhenFocused(true);
            ed.onReturnKey = [this] { apply(); };
            ed.onEscapeKey = [this] { dismiss(); };
            addAndMakeVisible(ed);
        };
        initBox(position_, timelinechrome::positionLabel(meters_.barAt(point_.beat),
                                                         meters_.beatInBar(point_.beat)),
                "0123456789.:");
        const bool hasValue = point_.kind != "trigger";
        const bool isRange = point_.kind == "range";
        if (hasValue) initBox(value_, valueText(point_.value), "0123456789.-eE");
        if (isRange) initBox(valueMax_, valueText(point_.valueMax), "0123456789.-eE");

        positionLabel_.setText(tr("auto-point-popup.at", "at"), juce::dontSendNotification);
        valueLabel_.setText(isRange ? tr("auto-point-popup.range", "range")
                                    : tr("auto-point-popup.value", "value"),
                            juce::dontSendNotification);
        suffix_.setText(unitSuffix(unit_), juce::dontSendNotification);
        for (auto* l : {&positionLabel_, &valueLabel_, &suffix_}) {
            l->setColour(juce::Label::textColourId, Palette::textDim);
            l->setFont(juce::FontOptions(11.5f));
            addAndMakeVisible(*l);
        }
        valueLabel_.setVisible(hasValue);
        suffix_.setVisible(hasValue && suffix_.getText().isNotEmpty());

        ok_.onClick = [this] { apply(); };
        addAndMakeVisible(ok_);
        setSize(hasValue ? (isRange ? 372 : 300) : 200, 62);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8, 6);
        title_.setBounds(r.removeFromTop(16));
        r.removeFromTop(4);
        auto row = r.removeFromTop(24);
        positionLabel_.setBounds(row.removeFromLeft(18));
        position_.setBounds(row.removeFromLeft(62));
        if (value_.isVisible()) {
            row.removeFromLeft(8);
            valueLabel_.setBounds(row.removeFromLeft(valueMax_.isVisible() ? 40 : 36));
            value_.setBounds(row.removeFromLeft(64));
            if (valueMax_.isVisible()) {
                row.removeFromLeft(4);
                valueMax_.setBounds(row.removeFromLeft(64));
            }
            if (suffix_.isVisible()) {
                row.removeFromLeft(3);
                suffix_.setBounds(row.removeFromLeft(30));
            }
        }
        ok_.setBounds(getWidth() - 50, value_.isVisible() ? value_.getY() : position_.getY(), 42, 24);
    }

    void parentHierarchyChanged() override {
        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<AutoPointPopup>(this)] {
                if (safe != nullptr && safe->isShowing())
                    (safe->value_.isVisible() ? safe->value_ : safe->position_).grabKeyboardFocus();
            });
    }

    void setTextsForTest(const juce::String& position, const juce::String& value,
                         const juce::String& valueMax) {
        position_.setText(position, juce::dontSendNotification);
        value_.setText(value, juce::dontSendNotification);
        valueMax_.setText(valueMax, juce::dontSendNotification);
    }
    void applyForTest() { apply(); }
    bool fitsForTest() const {
        const auto inside = getLocalBounds();
        const juce::Component* fields[] = {&position_, &value_, &valueMax_, &suffix_, &ok_};
        const juce::Component* all[] = {&position_, &value_, &valueMax_, &suffix_, &ok_,
                                        &positionLabel_, &valueLabel_};
        for (const auto* c : all) {
            if (!c->isVisible()) continue;
            if (!inside.contains(c->getBounds())) return false;
            for (const auto* o : fields) {
                if (o == c || !o->isVisible()) continue;
                if (c->getBounds().intersects(o->getBounds())) return false;
            }
        }
        return true;
    }

private:
    juce::String valueText(double v) const {
        return SetValuePopup::textFor(unit_, v, min_, max_, point_.kind == "step" ? 1.0 : 0.0);
    }

    void apply() {
        Point out = point_;
        double beat = 0.0;
        if (timelinechrome::parsePositionLabel(position_.getText(), meters_, beat)) out.beat = beat;
        if (value_.isVisible()) out.value = SetValuePopup::valueFor(unit_, value_.getText(), min_, max_);
        if (valueMax_.isVisible())
            out.valueMax = juce::jlimit(out.value, max_,
                                        SetValuePopup::valueFor(unit_, valueMax_.getText(), min_, max_));
        else out.valueMax = out.value;
        if (onApply_) onApply_(out);
        dismiss();
    }

    void dismiss() {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
    }

    Point point_;
    double min_, max_;
    Unit unit_;
    MeterMap meters_;
    std::function<void(Point)> onApply_;
    juce::Label title_, positionLabel_, valueLabel_, suffix_;
    juce::TextEditor position_, value_, valueMax_;
    juce::TextButton ok_{"OK"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoPointPopup)
};

}
