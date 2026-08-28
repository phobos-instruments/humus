#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

namespace hum {

class MenuStylePreviewCard : public juce::Component {
public:
    bool modern = false;
    bool selected = false;
    std::function<void()> onSelect;

    void mouseDown(const juce::MouseEvent&) override {
        if (onSelect) onSelect();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(1.5f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 8.0f);
        g.setColour(selected ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r, 8.0f, selected ? 2.0f : 1.0f);

        auto area = getLocalBounds().reduced(12);
        auto caption = area.removeFromBottom(34);
        auto mock = area.toFloat().reduced(4.0f);
        if (modern) drawModernMock(g, mock);
        else drawClassicMock(g, mock);

        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(modern ? "Modern cards" : "Classic menus",
                   caption.removeFromTop(17), juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(modern ? "Cards with breadcrumbs and search"
                          : "Nested submenus, classic-style",
                   caption, juce::Justification::centredLeft);
    }

private:
    static void drawClassicMock(juce::Graphics& g, juce::Rectangle<float> r) {
        const float rowH = juce::jmax(6.0f, r.getHeight() / 9.0f);
        auto menu = r.withWidth(r.getWidth() * 0.52f).withTrimmedBottom(r.getHeight() * 0.18f);
        auto sub = r.withTrimmedLeft(r.getWidth() * 0.44f)
                       .withTrimmedTop(r.getHeight() * 0.22f);
        for (const auto* p : {&menu, &sub}) {
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(*p, 4.0f);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(*p, 4.0f, 1.0f);
        }
        auto rows = menu.reduced(5.0f);
        const int n = juce::jmax(3, (int) (rows.getHeight() / rowH));
        for (int i = 0; i < n; ++i) {
            auto row = rows.removeFromTop(rowH).withTrimmedBottom(3.0f);
            const bool hot = i == 2;
            g.setColour(hot ? Palette::accent : Palette::textDim.withAlpha(0.55f));
            g.fillRoundedRectangle(row.withWidth(row.getWidth() * (hot ? 0.85f : 0.6f)), 2.0f);
            if (hot) {
                g.setColour(Palette::accent);
                juce::Path chev;
                const float cx = row.getRight() - 4.0f, cy = row.getCentreY();
                chev.addTriangle(cx, cy - 3.0f, cx, cy + 3.0f, cx + 4.0f, cy);
                g.fillPath(chev);
            }
        }
        auto subRows = sub.reduced(5.0f);
        for (int i = 0; i < 4 && subRows.getHeight() > rowH; ++i) {
            auto row = subRows.removeFromTop(rowH).withTrimmedBottom(3.0f);
            g.setColour(Palette::textDim.withAlpha(0.55f));
            g.fillRoundedRectangle(row.withWidth(row.getWidth() * 0.7f), 2.0f);
        }
    }

    static void drawModernMock(juce::Graphics& g, juce::Rectangle<float> r) {
        auto crumbs = r.removeFromTop(r.getHeight() * 0.16f);
        float x = crumbs.getX();
        for (int i = 0; i < 3; ++i) {
            const float w = 22.0f + 8.0f * (float) i;
            g.setColour(i == 2 ? Palette::accent.withAlpha(0.35f) : Palette::panelLight);
            g.fillRoundedRectangle(x, crumbs.getY(), w, crumbs.getHeight() - 2.0f,
                                   crumbs.getHeight() * 0.5f);
            x += w + 8.0f;
        }
        r.removeFromTop(4.0f);
        auto searchBar = r.removeFromTop(r.getHeight() * 0.18f);
        g.setColour(Palette::panelLight);
        g.fillRoundedRectangle(searchBar, 3.0f);
        g.setColour(Palette::textDim);
        g.drawEllipse(searchBar.getX() + 5.0f, searchBar.getCentreY() - 3.5f, 6.0f, 6.0f, 1.4f);
        r.removeFromTop(5.0f);
        const float gap = 5.0f;
        const float cw = (r.getWidth() - 2.0f * gap) / 3.0f;
        const float ch = (r.getHeight() - gap) / 2.0f;
        for (int row = 0; row < 2; ++row)
            for (int col = 0; col < 3; ++col) {
                const juce::Rectangle<float> card(r.getX() + (float) col * (cw + gap),
                                                  r.getY() + (float) row * (ch + gap), cw, ch);
                g.setColour(Palette::panelLight);
                g.fillRoundedRectangle(card, 3.0f);
                const auto tile = card.reduced(3.0f).removeFromLeft(ch - 6.0f);
                g.setColour(Palette::accent
                                .withRotatedHue(((float) (row * 3 + col) - 2.5f) * 0.03f)
                                .withAlpha(0.4f));
                g.fillRoundedRectangle(tile, 2.5f);
                g.setColour(Palette::textDim.withAlpha(0.6f));
                g.fillRoundedRectangle(tile.getRight() + 3.0f, card.getCentreY() - 1.5f,
                                       juce::jmax(4.0f, card.getRight() - tile.getRight() - 8.0f),
                                       3.0f, 1.5f);
            }
    }
};

}
