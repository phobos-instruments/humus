#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "gui/OrganismEditor.h"
#include "gui/EngineHost.h"
#include "gui/SetValuePopup.h"
#include "gui/FineDrag.h"
#include "gui/LookAndFeel.h"
#include "gui/ParamReset.h"

namespace hum {

class RangeSlider : public juce::Component {
public:
    enum class DragTarget { None, Low, High, Both };

    RangeSlider(EngineHost& host, std::string organism, std::string param,
                double min, double max, int decimals = 1, bool logarithmic = false)
        : host_(host), name_(std::move(organism)), param_(std::move(param)),
          min_(min), max_(max), decimals_(decimals),
          log_(logarithmic && min > 0.0 && max > min) {
        setOpaque(false);
        lo_ = host_.liveParamValue(name_, param_);
        hi_ = host_.liveParamMax(name_, param_);
    }

    std::function<void()> onAutomationChanged;

    void refresh() {
        double lo = host_.liveParamValue(name_, param_);
        double hi = host_.liveParamMax(name_, param_);
        if (lo_ != lo || hi_ != hi) { lo_ = lo; hi_ = hi; repaint(); }
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const float cx = r.getCentreX();
        const float top = trackTop();
        const float bottom = trackBottom();

        const float yLow  = valueToY(lo_, top, bottom);
        const float yHigh = valueToY(hi_, top, bottom);

        paintFaderGroove(g, r.withTop(top).withBottom(bottom), yHigh, yLow, false);

        const float halfH = 6.5f;
        const float w = juce::jlimit(12.0f, 30.0f, r.getWidth() - 6.0f);
        const bool both = dragging_ == DragTarget::Both;
        juce::Rectangle<float> topHalf(cx - w * 0.5f, yHigh - halfH, w, halfH);
        juce::Rectangle<float> botHalf(cx - w * 0.5f, yLow, w, halfH);
        drawFaderPot(g, topHalf, yHigh - halfH, yHigh + halfH, yHigh,
                     both || dragging_ == DragTarget::High, true, false, Palette::text);
        drawFaderPot(g, botHalf, yLow - halfH, yLow + halfH, yLow,
                     both || dragging_ == DragTarget::Low, false, true, Palette::text);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        refresh();
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, param_, e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        const float top = trackTop();
        const float bottom = trackBottom();
        const float yLow = valueToY(lo_, top, bottom);
        const float yHigh = valueToY(hi_, top, bottom);
        const float ey = (float) e.y;
        const float gap = yLow - yHigh;
        const float thumbR = 11.0f;

        anchorY_ = ey;
        anchorLo_ = lo_;
        anchorHi_ = hi_;

        if (gap >= 24.0f) {
            if (ey < yHigh + thumbR)      dragging_ = DragTarget::High;
            else if (ey > yLow - thumbR)  dragging_ = DragTarget::Low;
            else                          dragging_ = DragTarget::Both;
        } else {
            const float midY = 0.5f * (yHigh + yLow);
            const float H = std::max(15.0f, gap * 0.5f + 9.0f);
            if (ey < midY - H / 3.0f)      dragging_ = DragTarget::High;
            else if (ey > midY + H / 3.0f) dragging_ = DragTarget::Low;
            else                           dragging_ = DragTarget::Both;
        }
        host_.pushUndo();
        lastY_ = (float) e.y;
        if (dragging_ != DragTarget::Both && !e.mods.isShiftDown())
            dragAt((float) e.y, top, bottom);
        showBubble(0);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging_ == DragTarget::None) return;
        if (e.mods.isShiftDown()) dragFineBy(lastY_ - (float) e.y, trackTop(), trackBottom());
        else dragAt(e.y, trackTop(), trackBottom());
        lastY_ = (float) e.y;
        showBubble(0);
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (dragging_ != DragTarget::None) {
            dragging_ = DragTarget::None;
            bubble_.setVisible(false);
            repaint();
        }
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        SetValuePopup::showRange(
            getScreenBounds(), juce::String(param_), lo_, hi_, min_, max_,
            [safe = juce::Component::SafePointer<RangeSlider>(this)](double lo, double hi) {
                if (safe == nullptr) return;
                safe->host_.pushUndo();
                safe->lo_ = lo;
                safe->hi_ = hi;
                safe->push();
            });
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        double delta = (w.deltaY != 0.0 ? w.deltaY : w.deltaX);
        if (w.isReversed) delta = -delta;
        if (delta == 0.0) return;
        const bool fine = e.mods.isCtrlDown() || e.mods.isCommandDown() || e.mods.isShiftDown();
        const double frac = fine ? 0.001 : 0.01;
        const double range = max_ - min_;
        const double step = range * frac;
        lo_ = juce::jlimit(min_, max_, lo_ + delta * step);
        hi_ = juce::jlimit(min_, max_, hi_ + delta * step);
        push();
        showBubble(900);
    }

private:
    void showBubble(int timeoutMs) {
        auto* parent = getParentComponent();
        if (!parent) return;
        if (bubble_.getParentComponent() != parent) parent->addChildComponent(bubble_);
        juce::AttributedString msg(juce::String(lo_, decimals_) + juce::String::fromUTF8("  \xe2\x80\x93  ") +
                                   juce::String(hi_, decimals_));
        msg.setColour(Palette::text);
        msg.setJustification(juce::Justification::centred);
        msg.setFont(juce::Font(juce::FontOptions(12.0f)));
        const float top = trackTop(), bottom = trackBottom();
        const int yMid = (int) ((valueToY(lo_, top, bottom) + valueToY(hi_, top, bottom)) * 0.5f);
        auto a = parent->getLocalPoint(this, juce::Point<int>(getWidth() / 2, yMid));
        bubble_.showAt(juce::Rectangle<int>(a.x, a.y, 1, 1), msg, timeoutMs, false, false);
    }

    float trackTop() const { return (float) getLocalBounds().getY() + 9.0f; }
    float trackBottom() const { return (float) getLocalBounds().getBottom() - 9.0f; }

    double valueToFrac(double v) const {
        if (log_) return std::log(juce::jlimit(min_, max_, v) / min_) / std::log(max_ / min_);
        return (v - min_) / (max_ - min_);
    }
    double fracToValue(double f) const {
        f = juce::jlimit(0.0, 1.0, f);
        if (log_) return min_ * std::pow(max_ / min_, f);
        return min_ + f * (max_ - min_);
    }

    float valueToY(double v, float top, float bottom) const {
        double f = juce::jlimit(0.0, 1.0, valueToFrac(v));
        return bottom - (float) f * (bottom - top);
    }

    double yToValue(float y, float top, float bottom) const {
        double f = (bottom - y) / (bottom - top);
        return fracToValue(f);
    }

    void dragFineBy(float dy, float top, float bottom) {
        const double denom = (double) (bottom - top);
        if (denom <= 0.0) return;
        const double dFrac = (double) dy / denom / fineDragFactor();
        double fLo = valueToFrac(lo_), fHi = valueToFrac(hi_);
        if (dragging_ == DragTarget::Both) {
            const double width = fHi - fLo;
            double nLo = fLo + dFrac, nHi = fHi + dFrac;
            if (nLo < 0.0) { nLo = 0.0; nHi = width; }
            if (nHi > 1.0) { nHi = 1.0; nLo = 1.0 - width; }
            lo_ = fracToValue(nLo);
            hi_ = fracToValue(nHi);
        } else if (dragging_ == DragTarget::Low) {
            lo_ = fracToValue(juce::jlimit(0.0, fHi, fLo + dFrac));
        } else if (dragging_ == DragTarget::High) {
            hi_ = fracToValue(juce::jlimit(fLo, 1.0, fHi + dFrac));
        }
        push();
    }

    void dragAt(float y, float top, float bottom) {
        if (dragging_ == DragTarget::Both) {
            const double denom = (double) (bottom - top);
            if (denom <= 0.0) return;
            const double dFrac = (double) (anchorY_ - y) / denom;
            double fLo = valueToFrac(anchorLo_);
            double fHi = valueToFrac(anchorHi_);
            const double width = fHi - fLo;
            double nLo = fLo + dFrac, nHi = fHi + dFrac;
            if (nLo < 0.0) { nLo = 0.0; nHi = width; }
            if (nHi > 1.0) { nHi = 1.0; nLo = 1.0 - width; }
            lo_ = fracToValue(nLo);
            hi_ = fracToValue(nHi);
            push();
            return;
        }
        double v = yToValue(y, top, bottom);
        if (dragging_ == DragTarget::Low)  lo_ = std::min(v, hi_);
        else if (dragging_ == DragTarget::High) hi_ = std::max(v, lo_);
        push();
    }

    void push() {
        host_.setParamRange(name_, param_, lo_, hi_);
        repaint();
    }

    EngineHost& host_;
    std::string name_, param_;
    double min_, max_;
    int decimals_ = 1;
    bool log_ = false;
    double lo_ = 0.0, hi_ = 1.0;
    DragTarget dragging_ = DragTarget::None;
    float anchorY_ = 0.0f;
    float lastY_ = 0.0f;
    double anchorLo_ = 0.0, anchorHi_ = 0.0;
    juce::BubbleMessageComponent bubble_;
};

}
