// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

#include "hum/Meter.h"

namespace hum {

class ClockReadout : public juce::Component {
public:
    void setPosition(int bar, double beat, double seconds) {
        const int s = (int) seconds;
        if (bar != bar_ || (int) (beat * 100) != (int) (beat_ * 100) || s != secs_) {
            bar_ = bar; beat_ = beat; secs_ = s; repaint();
        }
    }
    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds();
        auto barsArea = area.removeFromLeft(area.getWidth() * 11 / 20);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 15.0f, juce::Font::plain));
        g.drawText(juce::String(bar_) + "-" + juce::String(beat_, 2),
                   barsArea, juce::Justification::centred, false);
        const int m = secs_ / 60, s = secs_ % 60, h = m / 60;
        auto z2 = [](int v) { return (v < 10 ? "0" : "") + juce::String(v); };
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
        g.drawText(h > 0 ? juce::String(h) + ":" + z2(m % 60) + ":" + z2(s)
                         : juce::String(m) + ":" + z2(s),
                   area, juce::Justification::centred, false);
    }
private:
    int bar_ = 1;
    double beat_ = 1.0;
    int secs_ = 0;
};

class TransportMeter : public juce::Component {
public:
    void push(float l, float r) {
        const float decay = 0.85f;
        float nl = juce::jmax(l, disp_[0] * decay);
        float nr = juce::jmax(r, disp_[1] * decay);
        if (nl < 1.0e-4f) nl = 0.0f;
        if (nr < 1.0e-4f) nr = 0.0f;
        if (nl == disp_[0] && nr == disp_[1]) return;
        disp_[0] = nl;
        disp_[1] = nr;
        repaint();
    }
    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds().reduced(1);
        int gap = 2;
        int barH = (area.getHeight() - gap) / 2;
        drawBar(g, area.removeFromTop(barH), disp_[0]);
        area.removeFromTop(gap);
        drawBar(g, area.removeFromTop(barH), disp_[1]);
    }
private:
    void drawBar(juce::Graphics& g, juce::Rectangle<int> r, float level) {
        g.setColour(Palette::background);
        g.fillRect(r);
        int w = juce::roundToInt((float) r.getWidth() * juce::jlimit(0.0f, 1.0f, level));
        auto filled = r.withWidth(w);
        juce::Colour c = level > 0.95f ? ink::state::danger
                       : level > 0.7f  ? ink::state::caution
                                       : Palette::accent;
        g.setColour(c);
        g.fillRect(filled);
        g.setColour(Palette::border);
        g.drawRect(r, 1);
    }
    float disp_[2] {0.0f, 0.0f};
};

class TimeSigChip : public juce::Component, public juce::SettableTooltipClient {
public:
    TimeSigChip() {
        setTooltip(juce::String::fromUTF8(
            "Time signature - click for the common meters, Custom for any other "
            "(13/16, 4/1), or automate it to change meter along the song"));
    }
    std::function<Meter()> get;
    std::function<void(Meter)> set;
    std::function<bool()> automated;
    std::function<void(bool)> setAutomated;

    static constexpr int kCustomItem = 1000;
    static constexpr int kAutomateItem = 1001;

    static std::vector<Meter> commonMeters() {
        return {{4, 4}, {3, 4}, {2, 4}, {5, 4}, {7, 4}, {2, 2}, {3, 2}, {6, 8},
                {7, 8}, {9, 8}, {12, 8}, {5, 16}, {7, 16}, {4, 1}};
    }

    Meter current() const { return get ? get() : Meter{}; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(isMouseOver() ? Palette::panelLight : Palette::panel);
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(automated && automated() ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r, 3.0f, 1.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                    12.0f, juce::Font::plain));
        g.drawText(juce::String(meterText(current())), getLocalBounds(),
                   juce::Justification::centred, false);
    }
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
    void mouseDown(const juce::MouseEvent&) override {
        juce::PopupMenu m;
        const Meter cur = current();
        const auto common = commonMeters();
        for (size_t i = 0; i < common.size(); ++i)
            m.addItem((int) i + 1, juce::String(meterText(common[i])), true, common[i] == cur);
        m.addSeparator();
        m.addItem(kCustomItem, tr("transport-widgets.custom-meter", "Custom..."));
        if (setAutomated) {
            m.addSeparator();
            m.addItem(kAutomateItem, tr("transport-widgets.automate-meter", "Automate the meter"),
                      true, automated && automated());
        }
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                        [this, common](int r) { onMenu(r, common); });
    }

private:
    void onMenu(int r, const std::vector<Meter>& common) {
        if (r == kAutomateItem) { if (setAutomated) setAutomated(!(automated && automated())); repaint(); return; }
        if (r == kCustomItem) { askCustom(); return; }
        if (r > 0 && (size_t) r <= common.size() && set) { set(common[(size_t) r - 1]); repaint(); }
    }

    void askCustom() {
        auto* w = new juce::AlertWindow(tr("transport-widgets.custom-meter-title", "Time signature"),
                                        tr("transport-widgets.custom-meter-help",
                                           "Beats per bar / beat unit, for example 13/16 or 4/1"),
                                        juce::MessageBoxIconType::NoIcon);
        w->addTextEditor("meter", juce::String(meterText(current())));
        w->addButton(tr("transport-widgets.ok", "OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton(tr("transport-widgets.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int result) {
            std::unique_ptr<juce::AlertWindow> owned(w);
            if (result != 1) return;
            Meter m;
            if (parseMeterText(w->getTextEditorContents("meter").trim().toStdString(), m) && set) {
                set(m);
                repaint();
            }
        }), false);
    }
};

}
