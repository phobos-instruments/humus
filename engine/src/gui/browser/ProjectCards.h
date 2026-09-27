// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserModel.h"
#include "gui/browser/BrowserPaint.h"
#include "gui/common/Localisation.h"

namespace hum::browser {

class ProjectCards : public juce::Component {
public:
    static constexpr int kCardW = 220, kCardH = 124, kSmallW = 190, kSmallH = 58, kGap = 12, kPad = 14, kStripH = 6;

    void setCompact(bool compact) {
        compact_ = compact;
        repaint();
    }
    bool compact() const { return compact_; }
    int cardW() const { return compact_ ? kSmallW : kCardW; }
    int cardH() const { return compact_ ? kSmallH : kCardH; }

    std::function<void()> onSelection;
    std::function<void(int row)> onOpen;
    std::function<void(int row, juce::Point<int>)> onMenu;
    std::function<void(const std::string& path, int stars)> onRate;

    explicit ProjectCards(BrowserModel& model) : model_(model) {}

    int columns(int width) const { return std::max(1, (width - kGap) / (cardW() + kGap)); }
    int heightFor(int width) const {
        const int n = (int) model_.rows().size();
        const int rows = (n + columns(width) - 1) / columns(width);
        return kGap + rows * (cardH() + kGap);
    }
    juce::Rectangle<int> cardBounds(int i) const {
        const int cols = columns(getWidth());
        return {kGap + (i % cols) * (cardW() + kGap), kGap + (i / cols) * (cardH() + kGap), cardW(), cardH()};
    }
    int cardAt(juce::Point<int> p) const {
        for (int i = 0; i < (int) model_.rows().size(); ++i)
            if (cardBounds(i).contains(p)) return i;
        return -1;
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::background);
        if (model_.rows().empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.5f));
            g.drawText(tr("browser.no-projects", "No projects yet. Save a patch and it lands here."),
                       getLocalBounds().reduced(kPad), juce::Justification::centredTop, true);
            return;
        }
        for (int i = 0; i < (int) model_.rows().size(); ++i) paintCard(g, i);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        const int i = cardAt(e.getPosition());
        if (i < 0) return;
        if (e.mods.isPopupMenu()) {
            if (onMenu) onMenu(i, e.getScreenPosition());
            return;
        }
        const auto stars = starsArea(i);
        if (stars.contains(e.position) && onRate) {
            const auto& entry = model_.rows()[(size_t) i];
            const int n = paint::starsAt(e.position.x, stars.getX());
            onRate(entry.path, n == entry.rating ? 0 : n);
            return;
        }
        model_.select(i, e.mods.isCommandDown(), e.mods.isShiftDown());
        if (onSelection) onSelection();
        repaint();
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override {
        const int i = cardAt(e.getPosition());
        if (i >= 0 && !starsArea(i).contains(e.position) && onOpen) onOpen(i);
    }

private:
    juce::Rectangle<float> starsArea(int i) const {
        const auto r = cardBounds(i).toFloat();
        return {r.getX() + kPad, r.getY() + (compact_ ? 32.0f : 48.0f), paint::kStarPitch * (float) kMaxRating, 14.0f};
    }

    static juce::Colour familyColour(char letter) {
        switch (letter) {
            case 'V': return Palette::familyAccent(Family::Voice);
            case 'T': return Palette::familyAccent(Family::Time);
            case 'M': return Palette::familyAccent(Family::Motion);
            case 'S': return Palette::familyAccent(Family::Sense);
            default: return Palette::textDim.withAlpha(alpha::muted);
        }
    }

    void paintCard(juce::Graphics& g, int i) const {
        const auto& e = model_.rows()[(size_t) i];
        const auto r = cardBounds(i).toFloat();
        const bool selected = model_.isSelected(i);
        g.setColour(selected ? Palette::panelLight : Palette::panel);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(selected ? Palette::accent : Palette::border.withAlpha(alpha::muted));
        g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, selected ? 1.5f : 1.0f);
        auto name = juce::String::fromUTF8(fileName(e.path).c_str());
        if (name.endsWithIgnoreCase(" Project")) name = name.dropLastCharacters(8);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(compact_ ? 12.5f : 14.0f, juce::Font::bold));
        g.drawFittedText(name, juce::Rectangle<int>((int) r.getX() + kPad, (int) r.getY() + (compact_ ? 9 : 12), cardW() - 2 * kPad, compact_ ? 18 : 34),
                         juce::Justification::topLeft, compact_ ? 1 : 2);
        paint::drawStars(g, e.rating, starsArea(i).getX(), starsArea(i).getCentreY(), selected);
        paint::drawHeart(g, {r.getRight() - kPad - 18.0f, starsArea(i).getY() - 2.0f, 18.0f, 18.0f}, e.favourite, false);
        if (compact_) return;
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.5f));
        juce::String line;
        if (e.facts.boxes > 0) line << e.facts.boxes << " " << tr("browser.boxes", "boxes");
        if (e.facts.bpm > 0.0) line << (line.isEmpty() ? "" : "  ") << juce::String(e.facts.bpm, 0) << " BPM";
        const auto when = e.lastUsed > 0 ? tr("browser.opened", "Opened") + " " + paint::dateText(e.lastUsed)
                                         : tr("browser.modified", "Modified") + " " + paint::dateText(e.modified);
        const auto where = juce::String::fromUTF8(whereText(e.path, Place{}, model_.roots()).c_str());
        if (where.isNotEmpty()) line << (line.isEmpty() ? "" : "  -  ") << where;
        const int x = (int) r.getX() + kPad, w = cardW() - 2 * kPad;
        g.drawText(line, x, (int) r.getY() + 66, w, 16, juce::Justification::centredLeft, true);
        g.drawText(when, x, (int) r.getY() + 82, w, 16, juce::Justification::centredLeft, true);
        const auto strip = juce::Rectangle<float>(r.getX() + kPad, r.getBottom() - 12.0f - kStripH, r.getWidth() - 2 * kPad, (float) kStripH);
        g.setColour(Palette::background);
        g.fillRoundedRectangle(strip, 2.0f);
        if (!e.facts.families.empty()) {
            const float cell = strip.getWidth() / (float) e.facts.families.size();
            for (size_t k = 0; k < e.facts.families.size(); ++k) {
                g.setColour(familyColour(e.facts.families[k]));
                g.fillRect(strip.getX() + cell * (float) k, strip.getY(), std::max(1.0f, cell - 1.0f), strip.getHeight());
            }
        }
    }

    BrowserModel& model_;
    bool compact_ = false;
};

}
