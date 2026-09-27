// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

struct CurveAxes {
    juce::String inLow = "0", inHigh = "127", outLow, outHigh;
};

class MappingCurveEditor : public juce::Component {
public:
    std::function<void()> onChanged;

    void setPoints(std::vector<std::pair<double, double>> pts) {
        points_ = std::move(pts);
        repaint();
    }
    const std::vector<std::pair<double, double>>& points() const { return points_; }
    void setAxes(const CurveAxes& axes) { axes_ = axes; repaint(); }
    void setLive(double input01, double waitingAt01) {
        if (std::abs(input01 - live_) < 1e-4 && std::abs(waitingAt01 - waiting_) < 1e-4) return;
        live_ = input01;
        waiting_ = waitingAt01;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto r = plot();
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r.expanded(1.0f), 4.0f);
        g.setColour(Palette::border.withAlpha(alpha::strong));
        g.drawRoundedRectangle(r.expanded(1.0f), 4.0f, 1.0f);
        g.setColour(Palette::border.withAlpha(alpha::muted));
        for (int q = 1; q < 4; ++q) {
            g.drawVerticalLine((int) (r.getX() + r.getWidth() * q / 4.0f), r.getY(), r.getBottom());
            g.drawHorizontalLine((int) (r.getY() + r.getHeight() * q / 4.0f), r.getX(), r.getRight());
        }
        paintAxes(g, r);
        const auto pts = effectivePoints();
        juce::Path line, fill;
        for (size_t i = 0; i < pts.size(); ++i) {
            const auto pos = toXY(pts[i]);
            if (i == 0) { line.startNewSubPath(pos); fill.startNewSubPath(r.getX(), r.getBottom()); }
            line.lineTo(pos);
            fill.lineTo(pos);
        }
        fill.lineTo(r.getRight(), r.getBottom());
        fill.closeSubPath();
        g.setColour(Palette::accent.withAlpha(alpha::wash));
        g.fillPath(fill);
        if (waiting_ >= 0.0) paintWaiting(g, r);
        g.setColour(Palette::accent);
        g.strokePath(line, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
        for (size_t i = 0; i < pts.size(); ++i) {
            const auto pos = toXY(pts[i]);
            const bool dying = (int) i == dragIndex_ && dragDeleting_;
            g.setColour(dying ? Palette::warnAmber() : Palette::background);
            g.fillEllipse(pos.x - 5.0f, pos.y - 5.0f, 10.0f, 10.0f);
            g.setColour(dying ? Palette::warnAmber() : Palette::accent);
            g.drawEllipse(pos.x - 5.0f, pos.y - 5.0f, 10.0f, 10.0f, 1.8f);
        }
        if (live_ >= 0.0) paintLive(g, r);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        auto pts = effectivePoints();
        dragIndex_ = -1;
        dragDeleting_ = false;
        for (size_t i = 0; i < pts.size(); ++i)
            if (toXY(pts[i]).getDistanceFrom(e.position) < 10.0f) { dragIndex_ = (int) i; break; }
        if (e.mods.isPopupMenu()) {
            const bool inner = dragIndex_ > 0 && dragIndex_ < (int) pts.size() - 1;
            dragIndex_ = -1;
            if (!inner) return;
            pts.erase(pts.begin() + (long) handleAt(e.position));
            points_ = std::move(pts);
            settle();
            return;
        }
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
        if (e.mods.isPopupMenu()) return;
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
        const auto r = plot();
        dragDeleting_ = !endpoint
            && (e.position.y < r.getY() - 16.0f || e.position.y > r.getBottom() + 16.0f);
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) return;
        if (dragDeleting_ && dragIndex_ > 0 && dragIndex_ < (int) points_.size() - 1)
            points_.erase(points_.begin() + dragIndex_);
        dragIndex_ = -1;
        dragDeleting_ = false;
        settle();
    }

    juce::Point<float> handlePositionForTest(int i) const {
        const auto pts = effectivePoints();
        return i >= 0 && i < (int) pts.size() ? toXY(pts[(size_t) i]) : juce::Point<float>();
    }

    int handleAt(juce::Point<float> at) const {
        const auto pts = effectivePoints();
        for (size_t i = 0; i < pts.size(); ++i)
            if (toXY(pts[i]).getDistanceFrom(at) < 10.0f) return (int) i;
        return -1;
    }

private:
    void settle() {
        if (points_.size() == 2 && std::abs(points_[0].second) < 1e-3
            && std::abs(points_[1].second - 1.0) < 1e-3)
            points_.clear();
        repaint();
        if (onChanged) onChanged();
    }

    static constexpr float kAxisW = 64.0f, kAxisH = 16.0f, kPad = 6.0f;

    juce::Rectangle<float> plot() const {
        return getLocalBounds().toFloat().withTrimmedLeft(kAxisW).withTrimmedBottom(kAxisH).reduced(kPad);
    }

    void paintAxes(juce::Graphics& g, juce::Rectangle<float> r) const {
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.5f));
        const float lx = 0.0f, lw = kAxisW - 4.0f;
        g.drawText(axes_.outHigh, juce::Rectangle<float>(lx, r.getY() - 2.0f, lw, 12.0f), juce::Justification::centredRight);
        g.drawText(axes_.outLow, juce::Rectangle<float>(lx, r.getBottom() - 10.0f, lw, 12.0f), juce::Justification::centredRight);
        const float by = r.getBottom() + kPad;
        g.drawText(axes_.inLow, juce::Rectangle<float>(r.getX(), by, 60.0f, 12.0f), juce::Justification::centredLeft);
        g.drawText(axes_.inHigh, juce::Rectangle<float>(r.getRight() - 60.0f, by, 60.0f, 12.0f), juce::Justification::centredRight);
    }

    void paintWaiting(juce::Graphics& g, juce::Rectangle<float> r) const {
        const float y = r.getBottom() - (float) waiting_ * r.getHeight();
        const float dashes[] = {4.0f, 4.0f};
        g.setColour(Palette::warnAmber().withAlpha(alpha::strong));
        g.drawDashedLine({r.getX(), y, r.getRight(), y}, dashes, 2, 1.2f);
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(tr("mapping-shape.waits-here", "the parameter is here - the knob takes over when it passes"),
                   juce::Rectangle<float>(r.getX() + 6.0f, y - 15.0f, r.getWidth() - 12.0f, 12.0f),
                   juce::Justification::centredRight);
    }

    void paintLive(juce::Graphics& g, juce::Rectangle<float> r) const {
        const double out = curveAt(live_);
        const auto pos = toXY({live_, out});
        g.setColour(Palette::text.withAlpha(alpha::veil));
        g.drawVerticalLine((int) pos.x, pos.y, r.getBottom());
        g.drawHorizontalLine((int) pos.y, r.getX(), pos.x);
        g.setColour(Palette::text);
        g.fillEllipse(pos.x - 4.0f, pos.y - 4.0f, 8.0f, 8.0f);
    }

    double curveAt(double t) const {
        const auto pts = effectivePoints();
        if (t <= pts.front().first) return pts.front().second;
        for (size_t i = 1; i < pts.size(); ++i)
            if (t <= pts[i].first) {
                const auto& a = pts[i - 1];
                const auto& b = pts[i];
                const double span = b.first - a.first;
                return a.second + (b.second - a.second) * (span > 1e-12 ? (t - a.first) / span : 1.0);
            }
        return pts.back().second;
    }

    std::vector<std::pair<double, double>> effectivePoints() const {
        if (points_.size() >= 2) return points_;
        return {{0.0, 0.0}, {1.0, 1.0}};
    }
    juce::Point<float> toXY(std::pair<double, double> p) const {
        const auto r = plot();
        return {r.getX() + (float) p.first * r.getWidth(),
                r.getY() + (float) (1.0 - p.second) * r.getHeight()};
    }
    std::pair<double, double> fromXY(juce::Point<float> pos) const {
        const auto r = plot();
        return {juce::jlimit(0.0, 1.0, (double) (pos.x - r.getX()) / juce::jmax(1.0f, r.getWidth())),
                juce::jlimit(0.0, 1.0, 1.0 - (double) (pos.y - r.getY()) / juce::jmax(1.0f, r.getHeight()))};
    }

    std::vector<std::pair<double, double>> points_;
    CurveAxes axes_;
    double live_ = -1.0, waiting_ = -1.0;
    int dragIndex_ = -1;
    bool dragDeleting_ = false;
};

}
