// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/FineDrag.h"

namespace hum {

class DragNumberEditor : public juce::TextEditor {
public:
    static constexpr int kDragThreshold = 3;
    static constexpr double kPixelsPerRange = 200.0;
    static constexpr double kPixelsPerStep = 4.0;
    static constexpr double kLogRatio = 50.0;

    std::function<double(const juce::String&)> parse = [](const juce::String& t) {
        return t.getDoubleValue();
    };
    std::function<juce::String(double)> format = [](double v) { return juce::String(v); };

    void setRange(double lo, double hi, bool integer, double perPixel = 0.0) {
        lo_ = std::min(lo, hi);
        hi_ = std::max(lo, hi);
        integer_ = integer;
        perPixel_ = perPixel;
        log_ = !integer && lo_ > 0.0 && hi_ / lo_ >= kLogRatio;
        setMouseCursor(hi_ > lo_ ? juce::MouseCursor::UpDownResizeCursor
                                 : juce::MouseCursor::IBeamCursor);
    }

    bool isScrubbing() const { return scrubbing_; }

    void mouseDown(const juce::MouseEvent& e) override {
        if (hasKeyboardFocus(true) || hi_ <= lo_) {
            juce::TextEditor::mouseDown(e);
            return;
        }
        pending_ = true;
        scrubbing_ = false;
        from_ = std::clamp(parse(getText()), lo_, hi_);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!pending_) {
            juce::TextEditor::mouseDrag(e);
            return;
        }
        const int dy = -e.getDistanceFromDragStartY();
        if (!scrubbing_ && std::abs(dy) < kDragThreshold) return;
        scrubbing_ = true;
        const double fine = e.mods.isShiftDown() ? fineDragFactor() : 1.0;
        setText(format(valueAt(dy / fine)), false);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (!pending_) {
            juce::TextEditor::mouseUp(e);
            return;
        }
        pending_ = false;
        if (scrubbing_) {
            scrubbing_ = false;
            if (onReturnKey) onReturnKey();
            return;
        }
        juce::TextEditor::mouseDown(e);
        juce::TextEditor::mouseUp(e);
    }

private:
    double valueAt(double pixels) const {
        double v;
        if (integer_) v = from_ + std::round(pixels / kPixelsPerStep);
        else if (perPixel_ > 0.0) v = from_ + pixels * perPixel_;
        else if (log_) v = from_ * std::pow(hi_ / lo_, pixels / kPixelsPerRange);
        else v = from_ + pixels * (hi_ - lo_) / kPixelsPerRange;
        return std::clamp(v, lo_, hi_);
    }

    double lo_ = 0.0, hi_ = 0.0, from_ = 0.0, perPixel_ = 0.0;
    bool integer_ = false, log_ = false, pending_ = false, scrubbing_ = false;
};

}
