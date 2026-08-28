#pragma once
#include <cmath>
#include <cstdint>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "hum/dsp/Formula.h"

namespace hum {

class FormulaBrick : public PolledBrick {
public:
    static double plotSpanSeconds(const FormulaProgram& p, double freq, double bpm) {
        const auto sh = formulaShape(p);
        const bool cycles = sh.role == FormulaRole::Voice || (sh.role == FormulaRole::Effect && !sh.timed);
        return cycles ? 4.0 / std::max(20.0, freq) : 60.0 / std::max(1.0, bpm);
    }

    FormulaBrick(EngineHost& host, std::string organism, std::string param)
        : PolledBrick(host, std::move(organism), 2), pn_(std::move(param)) {
        field_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                         13.0f, juce::Font::plain));
        field_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
        field_.setColour(juce::TextEditor::textColourId, Palette::text);
        field_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        field_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
        field_.setSelectAllWhenFocused(false);
        field_.onReturnKey = [this] { commit(); };
        field_.onFocusLost = [this] { commit(); };
        field_.onEscapeKey = [this] { pull(host_.liveParamText(name_, pn_)); };
        field_.onTextChange = [this] { preview(); };
        addAndMakeVisible(field_);
        pull(host_.liveParamText(name_, pn_));
    }

    void reloadValues() override { pull(host_.liveParamText(name_, pn_)); }
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
        g.setColour(Palette::border.withAlpha(0.4f));
        for (int q = 1; q < 4; ++q)
            g.drawVerticalLine(r.getX() + r.getWidth() * q / 4,
                               (float) r.getY() + 2.0f, (float) r.getBottom() - 2.0f);
        g.drawHorizontalLine(r.getCentreY(), (float) r.getX() + 2.0f,
                             (float) r.getRight() - 2.0f);
        if (prog_.valid()) {
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
            const auto sh = formulaShape(prog_);
            if (sh.role == FormulaRole::Effect) {
                g.setColour(Palette::textDim.withAlpha(0.35f));
                g.strokePath(trace(inSlots_), juce::PathStrokeType(1.0f));
            }
            g.setColour(Palette::accent.withAlpha(hasError_ ? 0.35f : 0.9f));
            g.strokePath(trace(slots_), juce::PathStrokeType(1.6f));
            g.setColour(Palette::textDim.withAlpha(0.7f));
            g.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
            g.drawText(formulaRoleName(sh.role), r.reduced(6, 3), juce::Justification::topRight);
        }
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 5.0f, 1.0f);

        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                    11.0f, juce::Font::plain));
        if (hasError_) {
            g.setColour(Palette::recordRed());
            juce::String line(err_.msg != nullptr ? err_.msg : "does not parse");
            if (err_.pos >= 0) {
                const float charW = juce::GlyphArrangement::getStringWidth(
                    field_.getFont(), "0");
                const int cx = field_.getX() + 6 + (int) (charW * (float) err_.pos);
                g.drawText("^", cx - 3, field_.getBottom() - 3, 10, 10,
                           juce::Justification::centred);
                line << "  (column " << (err_.pos + 1) << ")";
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
    static constexpr int kPlotN = 256;

    void pull(const std::string& text) {
        cachedText_ = text;
        liveEdit_ = false;
        if (!field_.hasKeyboardFocus(true))
            field_.setText(juce::String::fromUTF8(text.c_str()), juce::dontSendNotification);
        hasError_ = !compileFormula(text.c_str(), prog_, &err_);
        replot();
    }

    void commit() {
        const auto typed = field_.getText().toStdString();
        const bool pushed = liveEdit_;
        liveEdit_ = false;
        if (typed == cachedText_) return;
        cachedText_ = typed;
        if (!pushed) host_.pushUndo();
        host_.setParamText(name_, pn_, typed);
    }

    void preview() {
        const auto typed = field_.getText().toStdString();
        hasError_ = !compileFormula(typed.c_str(), prog_, &err_);
        if (!hasError_ && typed != cachedText_) {
            if (!liveEdit_) { host_.pushUndo(); liveEdit_ = true; }
            cachedText_ = typed;
            host_.setParamText(name_, pn_, typed);
        }
        replot();
    }

    void replot() {
        if (plotArea_.isEmpty() || !prog_.valid()) { repaint(); return; }
        FormulaEnv env;
        std::uint32_t rng = 0x9e3779b9u;
        float phase[FormulaProgram::kMaxOps] = {};
        env.rng = &rng;
        env.state = phase;
        const double bpm = host_.tempo();
        const double span = plotSpanSeconds(prog_, kx_[4], bpm);
        env.dt = (float) (span / kPlotN);
        const auto sh = formulaShape(prog_);
        const double probeHz = sh.role == FormulaRole::Effect && sh.timed ? 8.0 / span : 2.5 * kx_[4];
        env.v[fvNote] = (float) (69.0 + 12.0 * std::log2(std::max(20.0, kx_[4]) / 440.0));
        env.v[fvFreq] = (float) kx_[4];
        env.v[fvGate] = 1.0f;
        env.v[fvVel] = 1.0f;
        env.v[fvX] = (float) kx_[0];
        env.v[fvY] = (float) kx_[1];
        env.v[fvZ] = (float) kx_[2];
        env.v[fvW] = (float) kx_[3];
        env.v[fvBpm] = (float) bpm;
        env.v[fvSr] = (float) kPlotN;
        env.v[fvA] = env.v[fvB] = 0.0f;
        for (int i = 0, n = std::min(16384, (int) std::ceil(1.0 / env.dt)); i < n; ++i)
            env.v[fvPrev] = evalFormula(prog_, env);
        for (int i = 0; i < kPlotN; ++i) {
            const float t = (float) (span * i / (kPlotN - 1));
            env.v[fvT] = t;
            env.v[fvBeat] = t * (float) (std::max(1.0, bpm) / 60.0);
            if (sh.role == FormulaRole::Effect) {
                inSlots_[i] = 0.8f * std::sin((float) (juce::MathConstants<double>::twoPi * probeHz * t));
                env.v[fvA] = inSlots_[i];
                env.v[fvB] = 0.8f * std::sin((float) (juce::MathConstants<double>::twoPi * probeHz * 0.5 * t));
            }
            const float o = evalFormula(prog_, env);
            slots_[i] = o;
            env.v[fvPrev] = o;
        }
        repaint();
    }

    void poll() override {
        bool moved = false;
        for (int k = 0; k < 5; ++k) {
            const double v = host_.liveParamValue(name_, kKnobs[k]);
            if (std::abs(v - kx_[k]) > 1e-9) { kx_[k] = v; moved = true; }
        }
        const auto live = host_.liveParamText(name_, pn_);
        if (live != cachedText_ && !field_.hasKeyboardFocus(true)) {
            pull(live);
            return;
        }
        if (moved) replot();
    }

    static constexpr const char* kKnobs[5] = {"X", "Y", "Z", "W", "Freq"};
    std::string pn_;
    juce::TextEditor field_;
    juce::Rectangle<int> plotArea_, hintArea_;
    std::string cachedText_;
    FormulaProgram prog_;
    FormulaError err_;
    bool hasError_ = false;
    bool liveEdit_ = false;
    float slots_[kPlotN] = {};
    float inSlots_[kPlotN] = {};
    double kx_[5] = {0.5, 0.5, 0.5, 0.5, 220.0};
};

}
