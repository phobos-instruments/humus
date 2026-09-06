#pragma once
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ControlShape.h"
#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class MappingCurveEditor : public juce::Component {
public:
    std::function<void()> onChanged;

    void setPoints(std::vector<std::pair<double, double>> pts) {
        points_ = std::move(pts);
        repaint();
    }
    const std::vector<std::pair<double, double>>& points() const { return points_; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r, 3.0f, 1.0f);
        for (int q = 1; q < 4; ++q) {
            g.setColour(Palette::border.withAlpha(0.4f));
            g.drawVerticalLine((int) (r.getX() + r.getWidth() * q / 4.0f), r.getY(), r.getBottom());
            g.drawHorizontalLine((int) (r.getY() + r.getHeight() * q / 4.0f), r.getX(), r.getRight());
        }
        const auto pts = effectivePoints();
        juce::Path path;
        for (size_t i = 0; i < pts.size(); ++i) {
            const auto pos = toXY(pts[i]);
            if (i == 0) path.startNewSubPath(pos);
            else path.lineTo(pos);
        }
        g.setColour(Palette::accent);
        g.strokePath(path, juce::PathStrokeType(1.6f));
        for (size_t i = 0; i < pts.size(); ++i) {
            const auto pos = toXY(pts[i]);
            const bool dying = (int) i == dragIndex_ && dragDeleting_;
            g.setColour(dying ? Palette::textDim : Palette::accent);
            g.fillEllipse(pos.x - 3.5f, pos.y - 3.5f, 7.0f, 7.0f);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        auto pts = effectivePoints();
        dragIndex_ = -1;
        dragDeleting_ = false;
        for (size_t i = 0; i < pts.size(); ++i)
            if (toXY(pts[i]).getDistanceFrom(e.position) < 8.0f) { dragIndex_ = (int) i; break; }
        if (dragIndex_ < 0) {
            auto p = fromXY(e.position);
            size_t at = pts.size();
            for (size_t i = 0; i < pts.size(); ++i)
                if (pts[i].first > p.first) { at = i; break; }
            pts.insert(pts.begin() + (long) at, p);
            dragIndex_ = (int) at;
        }
        points_ = std::move(pts);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragIndex_ < 0 || dragIndex_ >= (int) points_.size()) return;
        const auto i = (size_t) dragIndex_;
        auto p = fromXY(e.position);
        const bool endpoint = i == 0 || i == points_.size() - 1;
        if (endpoint) p.first = i == 0 ? 0.0 : 1.0;
        else {
            const double lo = points_[i - 1].first + 1e-3;
            const double hi = points_[i + 1].first - 1e-3;
            p.first = juce::jlimit(lo, hi, p.first);
        }
        points_[i] = p;
        dragDeleting_ = !endpoint
            && (e.position.y < -16.0f || e.position.y > getHeight() + 16.0f);
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (dragDeleting_ && dragIndex_ > 0 && dragIndex_ < (int) points_.size() - 1)
            points_.erase(points_.begin() + dragIndex_);
        dragIndex_ = -1;
        dragDeleting_ = false;
        if (points_.size() == 2 && std::abs(points_[0].second) < 1e-3
            && std::abs(points_[1].second - 1.0) < 1e-3)
            points_.clear();
        repaint();
        if (onChanged) onChanged();
    }

private:
    std::vector<std::pair<double, double>> effectivePoints() const {
        if (points_.size() >= 2) return points_;
        return {{0.0, 0.0}, {1.0, 1.0}};
    }
    juce::Point<float> toXY(std::pair<double, double> p) const {
        return {2.0f + (float) p.first * (getWidth() - 4.0f),
                2.0f + (float) (1.0 - p.second) * (getHeight() - 4.0f)};
    }
    std::pair<double, double> fromXY(juce::Point<float> pos) const {
        return {juce::jlimit(0.0, 1.0, (pos.x - 2.0) / juce::jmax(1.0, getWidth() - 4.0)),
                juce::jlimit(0.0, 1.0, 1.0 - (pos.y - 2.0) / juce::jmax(1.0, getHeight() - 4.0))};
    }

    std::vector<std::pair<double, double>> points_;
    int dragIndex_ = -1;
    bool dragDeleting_ = false;
};

class MappingShapePanel : public juce::Component {
public:
    std::function<void(const ControlShape&)> onChanged;

    MappingShapePanel() {
        title_.setText(tr("mapping-shape.mapping", "Mapping"), juce::dontSendNotification);
        title_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        addAndMakeVisible(title_);

        smoothLabel_.setText(tr("mapping-shape.smoothing-s", "Smoothing (s)"), juce::dontSendNotification);
        smoothLabel_.setFont(juce::FontOptions(12.0f));
        addChildComponent(smoothLabel_);
        smooth_.setInputRestrictions(6, "0123456789.");
        smooth_.setJustification(juce::Justification::centred);
        auto commitSmooth = [this] {
            shape_.smoothing = juce::jlimit(0.0, 30.0, smooth_.getText().getDoubleValue());
            emit();
        };
        smooth_.onReturnKey = commitSmooth;
        smooth_.onFocusLost = commitSmooth;
        addChildComponent(smooth_);
        addChildComponent(curve_);
        curve_.onChanged = [this] {
            shape_.curve = curve_.points();
            emit();
        };

        thresholdLabel_.setText(tr("mapping-shape.threshold-0-127", "Threshold (0-127)"), juce::dontSendNotification);
        thresholdLabel_.setFont(juce::FontOptions(12.0f));
        addChildComponent(thresholdLabel_);
        threshold_.setInputRestrictions(3, "0123456789");
        threshold_.setJustification(juce::Justification::centred);
        auto commitThreshold = [this] {
            shape_.threshold = juce::jlimit(0, kMidiMax, threshold_.getText().getIntValue()) / kMidiMaxD;
            emit();
        };
        threshold_.onReturnKey = commitThreshold;
        threshold_.onFocusLost = commitThreshold;
        addChildComponent(threshold_);
        inverted_.setButtonText(tr("mapping-shape.inverted", "Inverted"));
        inverted_.onClick = [this] { shape_.inverted = inverted_.getToggleState(); emit(); };
        addChildComponent(inverted_);
        toggle_.setButtonText(tr("mapping-shape.toggle", "Toggle"));
        toggle_.onClick = [this] { shape_.toggle = toggle_.getToggleState(); emit(); };
        addChildComponent(toggle_);

        hint_.setFont(juce::FontOptions(11.0f));
        hint_.setColour(juce::Label::textColourId, Palette::textDim);
        hint_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(hint_);
    }

    void show(const ControlShape& shape, bool hasSource) {
        shape_ = shape;
        const bool sw = hasSource && shape.isSwitch;
        const bool num = hasSource && !shape.isSwitch;
        smoothLabel_.setVisible(num);
        smooth_.setVisible(num);
        curve_.setVisible(num);
        thresholdLabel_.setVisible(sw);
        threshold_.setVisible(sw);
        inverted_.setVisible(sw);
        toggle_.setVisible(sw);
        smooth_.setText(juce::String(shape.smoothing, 2), juce::dontSendNotification);
        curve_.setPoints(shape.curve);
        threshold_.setText(juce::String((int) std::lround(shape.threshold * kMidiMaxD)),
                           juce::dontSendNotification);
        inverted_.setToggleState(shape.inverted, juce::dontSendNotification);
        toggle_.setToggleState(shape.toggle, juce::dontSendNotification);
        hint_.setText(!hasSource ? ""
                      : sw ? "On above the threshold (Inverted flips). Toggle: each upward "
                             "crossing flips the state instead."
                           : "Curve: controller across, parameter up. Click the line to add "
                             "a handle, drag out to delete. Smoothing eases rapid changes.",
                      juce::dontSendNotification);
        resized();
        repaint();
    }

    void resized() override {
        auto area = getLocalBounds();
        title_.setBounds(area.removeFromTop(20));
        auto top = area.removeFromTop(24);
        if (smooth_.isVisible()) {
            smoothLabel_.setBounds(top.removeFromLeft(96));
            smooth_.setBounds(top.removeFromLeft(56).reduced(0, 2));
        } else {
            thresholdLabel_.setBounds(top.removeFromLeft(118));
            threshold_.setBounds(top.removeFromLeft(48).reduced(0, 2));
            top.removeFromLeft(12);
            inverted_.setBounds(top.removeFromLeft(90));
            toggle_.setBounds(top.removeFromLeft(80));
        }
        auto foot = area.removeFromBottom(28);
        hint_.setBounds(foot);
        curve_.setBounds(area.reduced(0, 3));
    }

private:
    void emit() { if (onChanged) onChanged(shape_); }

    ControlShape shape_;
    juce::Label title_, smoothLabel_, thresholdLabel_, hint_;
    juce::TextEditor smooth_, threshold_;
    MappingCurveEditor curve_;
    juce::ToggleButton inverted_, toggle_;
};

}
