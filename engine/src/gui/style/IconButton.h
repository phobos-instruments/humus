// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class IconButton : public juce::Button {
public:
    using Glyph = IconGlyph;

    explicit IconButton(Glyph g, const juce::String& tooltip)
        : juce::Button(tooltip), glyph_(g) { setTooltip(tooltip); }

    void setGlyph(Glyph g) { if (g != glyph_) { glyph_ = g; repaint(); } }
    void setOn(bool b) { if (b != on_) { on_ = b; repaint(); } }
    void setFloated(bool b) { if (b != floated_) { floated_ = b; repaint(); } }

    enum class Rail { None, Pane, Window };
    void setRail(Rail r) { if (r != rail_) { rail_ = r; repaint(); } }
    void setActiveColour(juce::Colour c) { activeCol_ = c; repaint(); }
    void setLead(bool b) { if (b != lead_) { lead_ = b; repaint(); } }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        if (rail_ != Rail::None) return paintRail(g, over, down);
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        const bool enabled = isEnabled();
        const bool active = on_ || getToggleState();
        const bool floating = floated_ && active;

        if (down || (over && enabled)) {
            g.setColour(Palette::panel.brighter(0.05f));
            g.fillRoundedRectangle(b, 5.0f);
        }
        const juce::Colour live = !activeCol_.isTransparent() ? activeCol_
                                : glyph_ == Glyph::Record    ? Palette::recordRed()
                                                             : Palette::accent;
        if (active && !floating) foxfire::bloom(g, b, live);
        if (floating) {
            g.setColour(Palette::accentDim);
            g.drawRoundedRectangle(b, 5.0f, 1.2f);
        }

        const juce::Colour fg = !enabled  ? Palette::border.brighter(0.08f)
                              : floating  ? Palette::text
                              : active    ? live
                              : (over)    ? Palette::text
                              : lead_     ? Palette::text
                                          : Palette::textDim;
        g.setColour(fg);
        drawIconGlyph(g, glyph_, b.reduced(b.getWidth() * 0.15f, b.getHeight() * 0.15f),
                      fg, enabled, active);
    }

private:
    void paintRail(juce::Graphics& g, bool over, bool down) {
        const bool enabled = isEnabled();
        const bool active = on_ || getToggleState();
        const bool floating = floated_ && active;
        auto full = getLocalBounds().toFloat();
        auto b = rail_ == Rail::Pane ? full.reduced(0.0f, 2.0f)
                                     : full.reduced(3.0f, 2.0f);

        if (rail_ == Rail::Window) {
            g.setColour(active ? Palette::panelLight
                               : (over && enabled ? Palette::panel.brighter(0.06f)
                                                  : Palette::panel));
            g.fillRoundedRectangle(b, 2.5f);
            const float band = 4.0f;
            juce::Path bandTop;
            bandTop.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), band + 2.0f,
                                        2.5f, 2.5f, true, true, false, false);
            g.setColour(active ? Palette::accent.withAlpha(alpha::heavy) : Palette::border.brighter(0.22f));
            g.saveState();
            g.reduceClipRegion(juce::Rectangle<int>((int) b.getX(), (int) b.getY(),
                                                    (int) b.getWidth(), (int) band));
            g.fillPath(bandTop);
            g.restoreState();
            g.setColour(active ? Palette::accentDim : Palette::border);
            g.drawRoundedRectangle(b.reduced(0.5f), 2.5f, 1.0f);
        } else if (down || (over && enabled)) {
            g.setColour(Palette::panel.brighter(0.10f));
            g.fillRect(b);
        }
        if (rail_ == Rail::Pane && active && !floating) foxfire::bloom(g, b, Palette::accent);

        if (rail_ == Rail::Window) {
        } else if (active && !floating) {
            g.setColour(Palette::accent);
            g.fillRect(full.getX(), b.getY() + 1.0f, 2.0f, b.getHeight() - 2.0f);
        } else if (floating) {
            g.setColour(Palette::accentDim);
            g.fillRect(full.getX(), b.getY() + 1.0f, 2.0f, 3.0f);
            g.fillRect(full.getX(), b.getBottom() - 4.0f, 2.0f, 3.0f);
        }

        const juce::Colour fg = !enabled ? Palette::border
                                         : (active ? Palette::text : Palette::textDim);
        g.setColour(fg);
        auto gr = rail_ == Rail::Pane
                      ? b.reduced(b.getWidth() * 0.16f, b.getHeight() * 0.16f)
                      : b.withTrimmedTop(5.0f).reduced(b.getWidth() * 0.19f,
                                                       b.getHeight() * 0.14f);
        drawIconGlyph(g, glyph_, gr, fg, enabled);
    }

    Glyph glyph_;
    bool on_ = false;
    bool floated_ = false;
    bool lead_ = false;
    Rail rail_ = Rail::None;
    juce::Colour activeCol_;
};

}
