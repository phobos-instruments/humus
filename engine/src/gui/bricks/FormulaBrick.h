// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/inputs/FormulaModel.h"

namespace hum {

class FormulaBrick : public PolledBrick {
public:
    FormulaBrick(BrickHost& host, std::string organism, std::string param, const Bindings& bound)
        : PolledBrick(host, organism, 2),
          formula_(host, organism, std::move(param),
                   {bound(bind::kXKnob), bound(bind::kYKnob), bound(bind::kZKnob), bound(bind::kWKnob),
                    bound(bind::kFreq)}) {
        field_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                         13.0f, juce::Font::plain));
        field_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
        field_.setColour(juce::TextEditor::textColourId, Palette::text);
        field_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        field_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
        field_.setSelectAllWhenFocused(false);
        field_.onReturnKey = [this] { commit(); };
        field_.onFocusLost = [this] { commit(); };
        field_.onEscapeKey = [this] { pull(formula_.liveText()); };
        field_.onTextChange = [this] { preview(); };
        addAndMakeVisible(field_);
        pull(formula_.liveText());
    }

    void reloadValues() override { pull(formula_.liveText()); }
    int preferredContentWidth() const override { return 584; }
    int preferredContentHeight(int) const override { return 168; }

    void resized() override {
        auto r = getLocalBounds();
        field_.setBounds(r.removeFromTop(24));
        r.removeFromTop(4);
        hintArea_ = r.removeFromBottom(28);
        plotArea_ = r;
        replot();
    }

    void paint(juce::Graphics& g) override {
        const auto r = plotArea_;
        if (r.isEmpty()) return;
        g.setColour(Palette::background.darker(0.15f));
        g.fillRoundedRectangle(r.toFloat(), 5.0f);
        g.setColour(Palette::border.withAlpha(alpha::dim));
        for (int q = 1; q < 4; ++q)
            g.drawVerticalLine(r.getX() + r.getWidth() * q / 4,
                               (float) r.getY() + 2.0f, (float) r.getBottom() - 2.0f);
        g.drawHorizontalLine(r.getCentreY(), (float) r.getX() + 2.0f,
                             (float) r.getRight() - 2.0f);
        const auto& err = formula_.error();
        const bool hasError = formula_.hasError();
        if (formula_.program().valid()) {
            const auto area = r.reduced(2).toFloat();
            auto yOf = [&](float v) {
                const float c = juce::jlimit(-1.0f, 1.0f, v);
                return area.getCentreY() - c * area.getHeight() * 0.5f;
            };
            auto trace = [&](const float* v) {
                juce::Path path;
                path.startNewSubPath(area.getX(), yOf(v[0]));
                for (int i = 1; i < kPlotN; ++i)
                    path.lineTo(area.getX() + area.getWidth() * (float) i / (kPlotN - 1), yOf(v[i]));
                return path;
            };
            const auto sh = formulaShape(formula_.program());
            if (sh.role == FormulaRole::Effect) {
                g.setColour(Palette::textDim.withAlpha(alpha::muted));
                g.strokePath(trace(formula_.input()), juce::PathStrokeType(1.0f));
            }
            g.setColour(Palette::accent.withAlpha(hasError ? 0.35f : 0.9f));
            g.strokePath(trace(formula_.output()), juce::PathStrokeType(1.6f));
            g.setColour(Palette::textDim.withAlpha(alpha::strong));
            g.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
            g.drawText(formulaRoleName(sh.role), r.reduced(6, 3), juce::Justification::topRight);
        }
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 5.0f, 1.0f);

        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                    11.0f, juce::Font::plain));
        if (hasError) {
            g.setColour(Palette::recordRed());
            juce::String line(err.msg != nullptr ? err.msg : "does not parse");
            if (err.pos >= 0) {
                const float charW = juce::GlyphArrangement::getStringWidth(
                    field_.getFont(), "0");
                const int cx = field_.getX() + 6 + (int) (charW * (float) err.pos);
                g.drawText("^", cx - 3, field_.getBottom() - 3, 10, 10,
                           juce::Justification::centred);
                line << "  (column " << (err.pos + 1) << ")";
            }
            g.drawText(line, hintArea_.reduced(4, 0), juce::Justification::centredLeft);
        } else {
            g.setColour(Palette::textDim);
            g.drawFittedText("a b  x y z w  t beat bpm sr prev  note freq gate vel  ch\n"
                             "sin cos tanh saw tri sqr pulse ph(hz) harm(hz,n,tilt) "
                             "env(gate,a,r) noise() if(c,a,b)",
                             hintArea_.reduced(4, 0), juce::Justification::centredLeft, 2);
        }
    }

private:
    static constexpr int kPlotN = input::FormulaModel::kPlotN;

    void pull(const std::string& text) {
        if (!field_.hasKeyboardFocus(true))
            field_.setText(juce::String::fromUTF8(text.c_str()), juce::dontSendNotification);
        formula_.pull(text);
        replot();
    }

    void commit() { formula_.commit(field_.getText().toStdString()); }

    void preview() {
        formula_.preview(field_.getText().toStdString());
        replot();
    }

    void replot() {
        if (!plotArea_.isEmpty() && formula_.program().valid()) formula_.plot();
        repaint();
    }

    void poll() override {
        const bool moved = formula_.pollKnobs();
        const auto live = formula_.liveText();
        if (live != formula_.cachedText() && !field_.hasKeyboardFocus(true)) {
            pull(live);
            return;
        }
        if (moved) replot();
    }

    input::FormulaModel formula_;
    juce::TextEditor field_;
    juce::Rectangle<int> plotArea_, hintArea_;
};

}
