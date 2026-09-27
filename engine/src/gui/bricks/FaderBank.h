// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/juce/JuceGeometry.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/host/BrickHost.h"
#include "gui/editor/FineDrag.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/ParamReset.h"
#include "gui/editor/inputs/FaderBankInput.h"

namespace hum {

class FaderBank : public juce::Component {
public:
    struct Spec { std::string param; juce::String label; double min = 0.0, max = 1.0; };

    FaderBank(BrickHost& host, std::string organism, std::vector<Spec> specs)
        : host_(host), name_(organism), labels_(labelsOf(specs)), bank_(host, organism, fadersOf(specs)) {}

    std::function<void()> onChange;
    std::function<void()> onAutomationChanged;

    void paint(juce::Graphics& g) override {
        const int n = bank_.count();
        if (n == 0) return;
        const float cw = (float) getWidth() / n;
        for (int i = 0; i < n; ++i) {
            const float x = i * cw;
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(labels_[(size_t) i], (int) x, 0, (int) cw, input::FaderBankInput::kLabelH,
                       juce::Justification::centred);
            const auto s = bank_.slot(i, getWidth(), getHeight());
            paintVerticalFader(g, toJuce(s), bank_.yOf(i, bank_.value(i), getWidth(), getHeight()), false);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        const int c = bank_.columnAt(e.x, getWidth());
        if (c < 0) return;
        if (e.mods.isPopupMenu()) {
            showAutomateMenu(host_, name_, bank_.fader(c).param, e.getScreenPosition(),
                             [this] { if (onAutomationChanged) onAutomationChanged(); });
            return;
        }
        bank_.press();
        fineCol_ = c;
        lastY_ = (float) e.y;
        if (!e.mods.isShiftDown()) paintAt(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isShiftDown()) {
            const int c = fineCol_ >= 0 ? fineCol_ : bank_.columnAt(e.x, getWidth());
            if (c >= 0) {
                bank_.dragFine(c, lastY_ - (float) e.y, getWidth(), getHeight(), fineDragFactor());
                changed();
            }
        } else {
            fineCol_ = bank_.columnAt(e.x, getWidth());
            paintAt(e);
        }
        lastY_ = (float) e.y;
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override {
        const int c = bank_.columnAt(e.x, getWidth());
        if (c >= 0 && bank_.reset(c)) changed();
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        const int c = bank_.columnAt(e.x, getWidth());
        if (c < 0) return;
        double delta = (w.deltaY != 0.0 ? w.deltaY : w.deltaX);
        if (w.isReversed) delta = -delta;
        if (delta == 0.0) return;
        const bool fine = e.mods.isCtrlDown() || e.mods.isCommandDown() || e.mods.isShiftDown();
        if (bank_.wheel(c, delta, fine)) changed();
    }

    void resized() override {}

private:
    static std::vector<juce::String> labelsOf(const std::vector<Spec>& specs) {
        std::vector<juce::String> out;
        for (const auto& s : specs) out.push_back(s.label);
        return out;
    }
    static std::vector<input::FaderBankInput::Fader> fadersOf(const std::vector<Spec>& specs) {
        std::vector<input::FaderBankInput::Fader> out;
        for (const auto& s : specs) out.push_back({s.param, s.min, s.max});
        return out;
    }

    void changed() {
        if (onChange) onChange();
        repaint();
    }

    void paintAt(const juce::MouseEvent& e) {
        if (bank_.drawAt(e.x, (float) e.y, getWidth(), getHeight())) changed();
    }

    BrickHost& host_;
    std::string name_;
    std::vector<juce::String> labels_;
    input::FaderBankInput bank_;
    int fineCol_ = -1;
    float lastY_ = 0.0f;
};

}
