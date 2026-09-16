// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class ZoomBar : public juce::Component {
public:
    static constexpr int kGut = 16, kLen = 140, kCap = 14;

    struct Target {
        std::function<bool()> shown;
        std::function<double()> norm;
        std::function<void(double)> setNorm;
        std::function<void(double)> step;
    };

    ZoomBar(int axis, Target target) : axis_(axis), target_(std::move(target)) {}

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    juce::Rectangle<int> capBounds(int i) const;

private:
    juce::Rectangle<int> slider() const;
    juce::Rectangle<int> groove() const;
    void setFrom(juce::Point<int> p);

    int axis_;
    Target target_;
    bool dragging_ = false;
    juce::Point<int> hover_{-1, -1};
};

}
