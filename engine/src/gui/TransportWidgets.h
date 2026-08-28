#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

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
        juce::Colour c = level > 0.95f ? juce::Colour(0xffe04030)
                       : level > 0.7f  ? juce::Colour(0xffd6c020)
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
    TimeSigChip() { setTooltip("Beats per bar"); }
    std::function<int()> get;
    std::function<void(int)> set;

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(isMouseOver() ? Palette::panelLight : Palette::panel);
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r, 3.0f, 1.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                    12.0f, juce::Font::plain));
        g.drawText(juce::String(get ? get() : 4) + "/4", getLocalBounds(),
                   juce::Justification::centred, false);
    }
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
    void mouseDown(const juce::MouseEvent&) override {
        juce::PopupMenu m;
        const int cur = get ? get() : 4;
        for (const int n : {2, 3, 4, 5, 6, 7, 9, 12})
            m.addItem(n, juce::String(n) + "/4", true, n == cur);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                        [this](int r) { if (r > 0 && set) { set(r); repaint(); } });
    }
};

}
