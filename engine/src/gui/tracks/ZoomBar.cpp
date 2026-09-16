// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/ZoomBar.h"

#include <algorithm>

#include "gui/style/LookAndFeel.h"

namespace hum {

juce::Rectangle<int> ZoomBar::slider() const {
    return axis_ == 0 ? juce::Rectangle<int>{getWidth() - kGut - kLen - 4, 1, kLen, kGut - 2}
                      : juce::Rectangle<int>{1, getHeight() - kLen - 4, kGut - 2, kLen};
}

juce::Rectangle<int> ZoomBar::capBounds(int i) const {
    auto s = slider();
    if (axis_ == 0) return i ? s.removeFromRight(kCap) : s.removeFromLeft(kCap);
    return i ? s.removeFromTop(kCap) : s.removeFromBottom(kCap);
}

juce::Rectangle<int> ZoomBar::groove() const {
    const auto s = slider();
    return axis_ == 0 ? s.reduced(kCap, 0) : s.reduced(0, kCap);
}

void ZoomBar::paint(juce::Graphics& g) {
    g.setColour(Palette::panel);
    g.fillRect(getLocalBounds());
    g.setColour(Palette::border);
    if (axis_ == 0) g.drawHorizontalLine(0, 0.0f, (float) getWidth());
    else g.drawVerticalLine(0, 0.0f, (float) getHeight());
    if (!target_.shown()) return;

    const auto gr = groove().toFloat();
    g.setColour(Palette::background);
    g.fillRoundedRectangle(gr.reduced(axis_ == 0 ? 0.0f : 4.0f, axis_ == 0 ? 4.0f : 0.0f), 2.0f);
    const float span = (axis_ == 0 ? gr.getWidth() : gr.getHeight()) - 16.0f;
    const float t = (float) target_.norm();
    const auto thumb = axis_ == 0
        ? juce::Rectangle<float>(gr.getX() + t * span, gr.getY() + 2.0f, 16.0f, gr.getHeight() - 4.0f)
        : juce::Rectangle<float>(gr.getX() + 2.0f, gr.getBottom() - 16.0f - t * span,
                                 gr.getWidth() - 4.0f, 16.0f);
    const bool hot = dragging_ || thumb.contains(hover_.toFloat());
    g.setColour(hot ? Palette::accent : Palette::textDim);
    g.fillRoundedRectangle(thumb, 2.0f);
    for (int i = 0; i < 2; ++i) {
        const auto cap = capBounds(i);
        g.setColour(Palette::panelLight.withAlpha(cap.contains(hover_) ? 0.95f : 0.55f));
        g.fillRoundedRectangle(cap.toFloat().reduced(2.0f), 2.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(i ? "+" : "-", cap, juce::Justification::centred, false);
    }
}

void ZoomBar::setFrom(juce::Point<int> p) {
    const auto gr = groove();
    target_.setNorm(axis_ == 0 ? (p.x - gr.getX() - 8.0) / std::max(1, gr.getWidth() - 16)
                               : (gr.getBottom() - p.y - 8.0) / std::max(1, gr.getHeight() - 16));
}

void ZoomBar::mouseDown(const juce::MouseEvent& e) {
    if (!target_.shown()) return;
    const auto p = e.getPosition();
    for (int i = 0; i < 2; ++i)
        if (capBounds(i).contains(p)) { target_.step(i ? 1.25 : 0.8); return; }
    if (groove().contains(p)) {
        dragging_ = true;
        setFrom(p);
    }
}

void ZoomBar::mouseDrag(const juce::MouseEvent& e) {
    if (dragging_) setFrom(e.getPosition());
}

void ZoomBar::mouseUp(const juce::MouseEvent&) {
    if (!dragging_) return;
    dragging_ = false;
    repaint();
}

void ZoomBar::mouseMove(const juce::MouseEvent& e) {
    hover_ = e.getPosition();
    repaint();
}

void ZoomBar::mouseExit(const juce::MouseEvent&) {
    hover_ = {-1, -1};
    repaint();
}

}
