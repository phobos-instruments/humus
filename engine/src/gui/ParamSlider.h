#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamUnit.h"
#include "core/ValueText.h"
#include "gui/Ember.h"
#include "gui/FineDrag.h"
#include "gui/FollowPick.h"
#include "gui/LookAndFeel.h"
#include "gui/SetValuePopup.h"
#include "gui/Localisation.h"

namespace hum {

class ParamSlider : public juce::Slider {
public:
    explicit ParamSlider(juce::Slider::SliderStyle style = juce::Slider::RotaryVerticalDrag,
                         juce::Slider::TextEntryBoxPosition textBox = juce::Slider::TextBoxBelow)
        : juce::Slider(style, textBox) {
        rotary_ = style == juce::Slider::RotaryVerticalDrag
               || style == juce::Slider::RotaryHorizontalVerticalDrag
               || style == juce::Slider::Rotary;
        if (rotary_) {
            setMouseDragSensitivity(420);
            setVelocityBasedMode(true);
            setVelocityModeParameters(0.7, 1, 0.09, false);
        } else {
            applyFineCrawl(*this);
        }
        setSliderSnapsToMousePosition(false);
        textFromValueFunction = [](double v) { return smartValueText(v); };
    }

    void setUnit(Unit u) {
        unit_ = u;
        if (u == Unit::None) {
            textFromValueFunction = [](double v) { return smartValueText(v); };
            valueFromTextFunction = nullptr;
        } else {
            const double lo = getMinimum(), hi = getMaximum();
            textFromValueFunction = [u, lo, hi](double v) { return unitFormat(u, v, lo, hi); };
            valueFromTextFunction = [u, lo, hi](const juce::String& t) {
                return unitParse(u, t, lo, hi);
            };
        }
        updateText();
    }
    Unit unit() const { return unit_; }

    const std::string& paramName() const { return param_; }

    juce::String editText() const {
        return SetValuePopup::textFor(unit_, getValue(), getMinimum(), getMaximum(),
                                      getInterval());
    }

    void configureScaling() {
        const double lo = getMinimum(), hi = getMaximum();
        logarithmic_ = (lo > 0.0 && hi / lo >= 50.0);
        if (logarithmic_)
            setSkewFactorFromMidPoint(std::sqrt(lo * hi));
    }

    std::function<void(juce::Point<int>)> onPopup;

    void valueChanged() override { ember::stamp(*this); }

    void setMeterLevel(float level) {
        const int q = level < 0.0f ? -1 : (int) (juce::jlimit(0.0f, 2.0f, level) * 40.0f);
        if (q == meterQ_) return;
        meterQ_ = q;
        getProperties().set("meter", q < 0 ? -1.0 : (double) q / 40.0);
        repaint();
    }
    float meterLevel() const { return meterQ_ < 0 ? -1.0f : (float) meterQ_ / 40.0f; }

    void setParamId(std::string organism, std::string param) {
        node_ = std::move(organism);
        param_ = std::move(param);
    }

    void setExternallyControlled(bool b) {
        if (externallyControlled_ == b) return;
        externallyControlled_ = b;
        repaint();
    }

    void setRollLocked(bool b) {
        if (rollLocked_ == b) return;
        rollLocked_ = b;
        repaint();
    }

    void paintOverChildren(juce::Graphics& g) override {
        if (externallyControlled_) {
            g.setColour(Palette::accent.withAlpha(0.55f));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.2f);
        }
        if (!rollLocked_) return;
        const auto r = getLocalBounds().toFloat();
        const float w = 7.0f, h = 5.0f;
        const float x = r.getRight() - w - 3.0f, y = r.getY() + 5.5f;
        g.setColour(Palette::textDim.withAlpha(0.9f));
        juce::Path shackle;
        shackle.addCentredArc(x + w * 0.5f, y, w * 0.28f, w * 0.28f, 0.0f,
                              -juce::MathConstants<float>::halfPi,
                              juce::MathConstants<float>::halfPi, true);
        g.strokePath(shackle, juce::PathStrokeType(1.0f));
        g.fillRoundedRectangle(x, y, w, h, 1.2f);
    }

    juce::String paramLabel;
    std::function<juce::String()> tooltipProvider;
    juce::String getTooltip() override {
        return tooltipProvider ? tooltipProvider() : juce::Slider::getTooltip();
    }

    static constexpr int kHoldDelay = 300, kHoldRate = 100, kHoldFastest = 20;

    void setStepping(double step, std::vector<double> ladder = {}) {
        step_ = step;
        ladder_ = std::move(ladder);
        std::sort(ladder_.begin(), ladder_.end());
    }

    void resized() override {
        juce::Slider::resized();
        if (getSliderStyle() != juce::Slider::IncDecButtons) return;
        for (auto* k : getChildren()) {
            auto* b = dynamic_cast<juce::Button*>(k);
            if (b == nullptr) continue;
            b->setRepeatSpeed(kHoldDelay, kHoldRate, kHoldFastest);
            if (step_ > 0.0 || !ladder_.empty()) {
                auto* tb = dynamic_cast<juce::TextButton*>(b);
                const bool up = tb == nullptr || tb->getButtonText() != "-";
                b->onClick = [this, up] { stepBy(up ? 1 : -1); };
            }
        }
    }

    void stepBy(int dir) {
        double v = getValue();
        if (ladder_.empty()) {
            v += dir * step_;
        } else {
            size_t i = 0;
            for (size_t k = 1; k < ladder_.size(); ++k)
                if (std::abs(ladder_[k] - v) < std::abs(ladder_[i] - v)) i = k;
            if (dir > 0 && ladder_[i] <= v + 1e-9) i = std::min(i + 1, ladder_.size() - 1);
            else if (dir < 0 && ladder_[i] >= v - 1e-9) i = i > 0 ? i - 1 : 0;
            v = ladder_[i];
        }
        setValue(juce::jlimit(getMinimum(), getMaximum(), v), juce::sendNotificationSync);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (auto* b = dynamic_cast<juce::Button*>(e.originalComponent))
            if (e.getDistanceFromDragStart() >= 10) b->setRepeatSpeed(kHoldDelay, 0, 0);
        if (rotary_ && e.mods.isShiftDown() != fineDrag_) {
            fineDrag_ = e.mods.isShiftDown();
            const double f = fineDrag_ ? fineDragFactor() : 1.0;
            setVelocityModeParameters(0.7 / f, 1, 0.09 / f, false);
        }
        juce::Slider::mouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (auto* b = dynamic_cast<juce::Button*>(e.originalComponent))
            b->setRepeatSpeed(kHoldDelay, kHoldRate, kHoldFastest);
        juce::Slider::mouseUp(e);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu() && followpick::take(node_, param_)) return;
        if (e.mods.isPopupMenu() && onPopup) {
            grabKeyboardFocus();
            onPopup(e.getScreenPosition());
            return;
        }
        if (rotary_) {
            fineDrag_ = false;
            setVelocityModeParameters(0.7, 1, 0.09, false);
        } else {
            applyFineCrawl(*this);
        }
        juce::Slider::mouseDown(e);
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override {
        if (dynamic_cast<juce::Button*>(e.originalComponent) != nullptr) return;
        if (e.mods.isAltDown()) { juce::Slider::mouseDoubleClick(e); return; }
        SetValuePopup::show(getScreenBounds(),
                            paramLabel.isNotEmpty() ? paramLabel : juce::String(tr("param-slider.set-value", "Set Value")),
                            getValue(), getMinimum(), getMaximum(), getInterval(), unit_,
                            paramLabel.containsIgnoreCase("freq"),
                            [safe = juce::Component::SafePointer<ParamSlider>(this)](double v) {
                                if (safe != nullptr)
                                    safe->setValue(v, juce::sendNotificationSync);
                            });
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        const bool adjust = e.mods.isCtrlDown() || e.mods.isCommandDown()
                         || e.mods.isAltDown() || e.mods.isShiftDown();
        if (!adjust)
            for (auto* p = getParentComponent(); p != nullptr; p = p->getParentComponent())
                if (auto* vp = dynamic_cast<juce::Viewport*>(p))
                    if (vp->useMouseWheelMoveIfNeeded(e.getEventRelativeTo(vp), w)) return;

        const double range = getMaximum() - getMinimum();
        if (range <= 0.0) { juce::Slider::mouseWheelMove(e, w); return; }

        double delta = (w.deltaY != 0.0 ? w.deltaY : w.deltaX);
        if (w.isReversed) delta = -delta;
        if (delta == 0.0) return;
        const int dir = delta > 0.0 ? 1 : -1;

        const bool fine = e.mods.isCtrlDown() || e.mods.isCommandDown() || e.mods.isShiftDown();
        const double frac = fine ? 0.001 : 0.01;

        double v = getValue();
        if (logarithmic_) {
            const double ratio = std::pow(getMaximum() / getMinimum(), frac);
            v = (dir > 0) ? v * ratio : v / ratio;
        } else {
            double step = range * frac;
            const double interval = getInterval();
            if (interval > 0.0) step = juce::jmax(step, interval);
            v += dir * step;
        }
        setValue(snapValue(v, juce::Slider::absoluteDrag), juce::sendNotificationSync);
    }

private:
    bool logarithmic_ = false;
    Unit unit_ = Unit::None;
    int meterQ_ = -1;
    std::string node_, param_;
    bool externallyControlled_ = false;
    bool rollLocked_ = false;
    bool rotary_ = false;
    bool fineDrag_ = false;
    double step_ = 0.0;
    std::vector<double> ladder_;
};

}
