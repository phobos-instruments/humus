// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/browser/BrowserPaint.h"
#include "gui/common/Localisation.h"

namespace hum::browser {

class BatchBar : public juce::Component {
public:
    static constexpr int kHeight = 36, kPad = 10, kStarsX = 120;

    std::function<void(int stars)> onRate;
    std::function<void()> onTag, onCollect, onClear;

    BatchBar() {
        for (auto* b : {&tag_, &collect_, &clear_}) addAndMakeVisible(*b);
        restyle();
        tag_.setButtonText(tr("browser.batch-tag", "Tag..."));
        collect_.setButtonText(tr("browser.add-to-collection", "Add to Collection"));
        clear_.setButtonText(tr("browser.batch-clear", "Clear"));
        tag_.onClick = [this] { if (onTag) onTag(); };
        collect_.onClick = [this] { if (onCollect) onCollect(); };
        clear_.onClick = [this] { if (onClear) onClear(); };
    }

    void restyle() {
        for (auto* b : {&tag_, &collect_, &clear_}) {
            b->setColour(juce::TextButton::buttonColourId, Palette::panel);
            b->setColour(juce::TextButton::textColourOffId, Palette::text);
        }
        repaint();
    }
    void lookAndFeelChanged() override { restyle(); }

    void setCount(int n) {
        count_ = n;
        repaint();
    }
    int count() const { return count_; }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panelLight.withAlpha(alpha::heavy));
        g.setColour(Palette::accent);
        g.fillRect(0, 0, 2, getHeight());
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        g.drawText(juce::String(count_) + " " + tr("browser.selected", "selected"), kPad, 0, kStarsX - kPad, getHeight(),
                   juce::Justification::centredLeft, false);
        paint::drawStars(g, 0, (float) kStarsX, (float) getHeight() * 0.5f, true);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(kPad, 6);
        clear_.setBounds(r.removeFromRight(64));
        r.removeFromRight(6);
        collect_.setBounds(r.removeFromRight(140));
        r.removeFromRight(6);
        tag_.setBounds(r.removeFromRight(70));
    }

    void mouseUp(const juce::MouseEvent& e) override {
        const float span = paint::kStarPitch * (float) kMaxRating;
        if (e.position.x >= (float) kStarsX && e.position.x < (float) kStarsX + span && onRate)
            onRate(paint::starsAt(e.position.x, (float) kStarsX));
    }

private:
    juce::TextButton tag_, collect_, clear_;
    int count_ = 0;
};

}
