#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/OrganismEditor.h"
#include "gui/EngineHost.h"
#include "gui/FineDrag.h"
#include "gui/LookAndFeel.h"
#include "gui/ParamReset.h"

namespace hum {

class FaderBank : public juce::Component {
public:
    struct Spec { std::string param; juce::String label; double min = 0.0, max = 1.0; };

    FaderBank(EngineHost& host, std::string organism, std::vector<Spec> specs)
        : host_(host), name_(std::move(organism)), specs_(std::move(specs)) {}

    std::function<void()> onChange;
    std::function<void()> onAutomationChanged;

    void paint(juce::Graphics& g) override {
        const int n = (int) specs_.size();
        if (n == 0) return;
        const float cw = (float) getWidth() / n;
        for (int i = 0; i < n; ++i) {
            const float x = i * cw;
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(specs_[(size_t) i].label, (int) x, 0, (int) cw, kLabelH,
                       juce::Justification::centred);
            paintVerticalFader(g, faderBounds(i), yFromValue(i, liveValue(i)), false);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        const int c = colAt(e.x);
        if (c < 0) return;
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, specs_[(size_t) c].param, e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        host_.pushUndo();
        fineCol_ = c;
        lastY_ = (float) e.y;
        if (!e.mods.isShiftDown()) paintAt(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isShiftDown()) {
            const int c = fineCol_ >= 0 ? fineCol_ : colAt(e.x);
            if (c >= 0) {
                const auto& s = specs_[(size_t) c];
                const auto fb = faderBounds(c);
                const double perPx = (s.max - s.min) / std::max(1.0f, fb.getHeight());
                const double v = juce::jlimit(
                    s.min, s.max, liveValue(c) + (lastY_ - (float) e.y) * perPx / fineDragFactor());
                host_.setParam(name_, s.param, v);
                if (onChange) onChange();
                repaint();
            }
        } else {
            fineCol_ = colAt(e.x);
            paintAt(e);
        }
        lastY_ = (float) e.y;
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override {
        const int c = colAt(e.x);
        if (c < 0) return;
        double def = 0.0, defMax = 0.0;
        if (!paramDefault(host_, name_, specs_[(size_t) c].param, def, defMax)) return;
        host_.editParam(name_, specs_[(size_t) c].param, def);
        if (onChange) onChange();
        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        const int c = colAt(e.x);
        if (c < 0) return;
        double delta = (w.deltaY != 0.0 ? w.deltaY : w.deltaX);
        if (w.isReversed) delta = -delta;
        if (delta == 0.0) return;
        const auto& s = specs_[(size_t) c];
        const double range = s.max - s.min;
        if (range <= 0.0) return;
        const bool fine = e.mods.isCtrlDown() || e.mods.isCommandDown() || e.mods.isShiftDown();
        const double step = range * (fine ? 0.001 : 0.01) * (delta > 0.0 ? 1.0 : -1.0);
        const double v = juce::jlimit(s.min, s.max, liveValue(c) + step);
        host_.editParam(name_, s.param, v);
        if (onChange) onChange();
        repaint();
    }

    void resized() override {}

private:
    static constexpr int kLabelH = 13;

    int colAt(int x) const {
        const int n = (int) specs_.size();
        if (n == 0 || getWidth() <= 0) return -1;
        return juce::jlimit(0, n - 1, x * n / getWidth());
    }
    juce::Rectangle<float> faderBounds(int i) const {
        const float cw = (float) getWidth() / (float) specs_.size();
        return {i * cw + cw * 0.2f, (float) (kLabelH + 4),
                cw * 0.6f, (float) (getHeight() - kLabelH - 8)};
    }
    double liveValue(int i) const { return host_.liveParamValue(name_, specs_[(size_t) i].param); }

    float yFromValue(int i, double v) const {
        auto fb = faderBounds(i);
        const auto& s = specs_[(size_t) i];
        const double f = (s.max > s.min) ? (v - s.min) / (s.max - s.min) : 0.0;
        return (float) (fb.getBottom() - juce::jlimit(0.0, 1.0, f) * fb.getHeight());
    }
    double valueFromY(int i, float y) const {
        auto fb = faderBounds(i);
        const auto& s = specs_[(size_t) i];
        const double f = juce::jlimit(0.0, 1.0, (double) ((fb.getBottom() - y) / fb.getHeight()));
        return s.min + f * (s.max - s.min);
    }
    void paintAt(const juce::MouseEvent& e) {
        const int c = colAt(e.x);
        if (c < 0) return;
        host_.setParam(name_, specs_[(size_t) c].param, valueFromY(c, (float) e.y));
        if (onChange) onChange();
        repaint();
    }

    EngineHost& host_;
    std::string name_;
    std::vector<Spec> specs_;
    int fineCol_ = -1;
    float lastY_ = 0.0f;
};

}
