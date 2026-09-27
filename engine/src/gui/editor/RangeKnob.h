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
#include "gui/editor/FineDrag.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/editor/SetValuePopup.h"
#include "gui/editor/inputs/RangeInput.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"
#include "gui/style/ParamMarks.h"
#include "gui/style/LookAndFeel.h"
#include "core/params/UnitText.h"

namespace hum {

class RangeKnob : public juce::Component {
public:
    static constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
    static constexpr float kEndAngle = juce::MathConstants<float>::pi * 2.8f;

    RangeKnob(BrickHost& host, std::string organism, std::string param, double min, double max,
              int decimals = 1, bool logarithmic = false, int family = -1, Unit unit = Unit::None)
        : host_(host), name_(std::move(organism)),
          range_(host, name_, std::move(param), min, max, logarithmic),
          decimals_(decimals), family_(family), unit_(unit) {
        setOpaque(false);
    }

    std::function<void()> onAutomationChanged;

    bool controlledForTest() const { return controlled_; }

    void setExternallyControlled(bool b) {
        if (controlled_ == b) return;
        controlled_ = b;
        repaint();
    }

    void paintOverChildren(juce::Graphics& g) override {
        paintParamMarks(g, getLocalBounds().toFloat(), controlled_, false);
    }

    void refresh() {
        if (range_.refresh()) repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto area = knobArea();
        const float radius = std::min(area.getWidth(), area.getHeight()) * 0.5f;
        const auto centre = area.getCentre();
        const float lineW = std::max(2.0f, radius * 0.16f);
        const float arcR = radius - lineW * 0.5f;
        const auto accent = family_ >= 0 ? Palette::familyAccent((Family) family_) : Palette::accent;

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, kStartAngle, kEndAngle, true);
        g.setColour(Palette::panelLight.withAlpha(alpha::nearOpaque));
        g.strokePath(track, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float a0 = angleOf(range_.low()), a1 = angleOf(range_.high());
        if (a1 - a0 > 0.005f) {
            juce::Path span;
            span.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, a0, a1, true);
            g.setColour(accent);
            g.strokePath(span, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        } else {
            const auto dot = pointAt(centre, arcR, a0);
            g.setColour(accent);
            g.fillEllipse(dot.x - lineW * 0.5f, dot.y - lineW * 0.5f, lineW, lineW);
        }

        knobFace(g, centre, arcR - lineW * 1.3f, 0.5f * (a0 + a1), accent);

        if (readingHeight() <= 0.0f) return;
        const auto below = getLocalBounds().removeFromBottom((int) readingHeight());
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(reading(), below, juce::Justification::centredTop, 1, 0.82f);
    }

    juce::String reading() const {
        auto lo = juce::String(unitText(unit_, range_.low(), range_.minimum(), range_.maximum()));
        if (range_.high() <= range_.low()) return lo;
        const auto hi = juce::String(unitText(unit_, range_.high(), range_.minimum(), range_.maximum()));
        if (const juce::String suffix(unitSuffix(unit_)); suffix.isNotEmpty() && lo.endsWith(suffix))
            lo = lo.dropLastCharacters(suffix.length()).trimEnd();
        return lo + juce::String::fromUTF8(" \xe2\x80\x93 ") + hi;
    }

    void mouseDown(const juce::MouseEvent& e) override {
        refresh();
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, range_.param(), e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        highEnd_ = (float) e.y < knobArea().getCentreY();
        range_.aimAt(input::RangeInput::Target::Both);
        last_ = e.position;
        repaint();
        showBubble(0);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (range_.target() == input::RangeInput::Target::None) return;
        const float reach = std::max(40.0f, (float) knobArea().getHeight() * 2.0f);
        const double fine = e.mods.isShiftDown() ? fineDragFactor() : 1.0;
        const double dUp = (double) (last_.y - e.position.y) / reach / fine;
        const double dSide = (double) (e.position.x - last_.x) / reach / fine;
        if (e.mods.isAltDown()) range_.nudgeEnd(highEnd_, dUp);
        else if (std::abs(dSide) > 0.0) range_.nudgeSpread(dSide);
        if (!e.mods.isAltDown() && std::abs(dUp) > 0.0) range_.nudgeCentre(dUp);
        last_ = e.position;
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
            [safe = juce::Component::SafePointer<RangeKnob>(this)](double lo, double hi) {
                if (safe == nullptr) return;
                safe->range_.setBoth(lo, hi);
                safe->repaint();
            });
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        double delta = (w.deltaY != 0.0 ? w.deltaY : w.deltaX);
        if (w.isReversed) delta = -delta;
        if (delta == 0.0) return;
        range_.wheel(delta, e.mods.isShiftDown());
        repaint();
        showBubble(900);
    }

    double lowForTest() const { return range_.low(); }
    double highForTest() const { return range_.high(); }

private:
    static juce::Point<float> pointAt(juce::Point<float> centre, float radius, float angle) {
        return {centre.x + radius * std::sin(angle), centre.y - radius * std::cos(angle)};
    }

    float angleOf(double value) const {
        const double span = range_.maximum() - range_.minimum();
        const double f = span > 0.0 ? (value - range_.minimum()) / span : 0.0;
        return kStartAngle + (float) std::clamp(f, 0.0, 1.0) * (kEndAngle - kStartAngle);
    }

    float readingHeight() const { return getHeight() >= 44 ? 16.0f : 0.0f; }

    juce::Rectangle<float> knobArea() const {
        const auto r = getLocalBounds().toFloat().withTrimmedBottom(readingHeight());
        const float side = std::min(r.getWidth(), r.getHeight());
        return r.withSizeKeepingCentre(side, side).reduced(2.0f);
    }

    void showBubble(int timeoutMs) {
        auto* parent = getParentComponent();
        if (!parent) return;
        if (bubble_.getParentComponent() != parent) parent->addChildComponent(bubble_);
        juce::AttributedString msg(reading());
        msg.setColour(Palette::text);
        msg.setJustification(juce::Justification::centred);
        msg.setFont(juce::Font(juce::FontOptions(12.0f)));
        const auto a = parent->getLocalPoint(this, juce::Point<int>(getWidth() / 2, 0));
        bubble_.showAt(juce::Rectangle<int>(a.x, a.y, 1, 1), msg, timeoutMs, false, false);
    }

    BrickHost& host_;
    std::string name_;
    input::RangeInput range_;
    int decimals_ = 1;
    int family_ = -1;
    Unit unit_ = Unit::None;
    bool controlled_ = false;
    bool highEnd_ = true;
    juce::Point<float> last_;
    juce::BubbleMessageComponent bubble_;
};

}
