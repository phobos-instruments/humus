// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/browser/BrowserPaint.h"
#include "gui/common/Localisation.h"

namespace hum::browser {

class BatchBar : public juce::Component {
public:
    static constexpr int kHeight = 36, kPad = 10, kStarsX = 120;

    std::function<void(int stars)> onRate;
    static constexpr int kTagW = 70, kCollectW = 140, kClearW = 64, kTrashW = 120, kGap = 6, kTrashGap = 16, kStarsGap = 8;

    std::function<void()> onTag, onCollect, onClear, onTrash;

    BatchBar() {
        for (auto* b : {&tag_, &collect_, &clear_, &trash_}) addAndMakeVisible(*b);
        restyle();
        tag_.setButtonText(tr("browser.batch-tag", "Tag..."));
        collect_.setButtonText(tr("browser.add-to-collection", "Add to Collection"));
        clear_.setButtonText(tr("browser.batch-clear", "Clear"));
        tag_.onClick = [this] { if (onTag) onTag(); };
        collect_.onClick = [this] { if (onCollect) onCollect(); };
        clear_.onClick = [this] { if (onClear) onClear(); };
        trash_.setButtonText(tr("browser.move-to-trash", "Move to Trash"));
        trash_.onClick = [this] { if (onTrash) onTrash(); };
    }

    void setTrashable(bool can) { trash_.setEnabled(can); }
    juce::TextButton& trashButton() { return trash_; }

    void restyle() {
        for (auto* b : {&tag_, &collect_, &clear_}) {
            b->setColour(juce::TextButton::buttonColourId, Palette::panel);
            b->setColour(juce::TextButton::textColourOffId, Palette::text);
        }
        paint::styleDanger(trash_);
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
        g.drawText(trn("browser.selected", "{n} selected", "{n} selected", count_), kPad, 0, kStarsX - kPad, getHeight(),
                   juce::Justification::centredLeft, false);
        paint::drawStars(g, 0, (float) kStarsX, (float) getHeight() * 0.5f, true);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(kPad, 6);
        r.setLeft(std::max(r.getX(), kStarsX + (int) (paint::kStarPitch * (float) kMaxRating) + kStarsGap));
        const int wanted = kTagW + kCollectW + kClearW + kTrashW + 2 * kGap + kTrashGap;
        const int squeeze = std::max(0, wanted - r.getWidth());
        trash_.setBounds(r.removeFromRight(kTrashW - squeeze / 2));
        r.removeFromRight(kTrashGap);
        clear_.setBounds(r.removeFromRight(kClearW));
        r.removeFromRight(kGap);
        collect_.setBounds(r.removeFromRight(kCollectW - (squeeze - squeeze / 2)));
        r.removeFromRight(kGap);
        tag_.setBounds(r.removeFromRight(kTagW));
    }

    void mouseUp(const juce::MouseEvent& e) override {
        const float span = paint::kStarPitch * (float) kMaxRating;
        if (e.position.x >= (float) kStarsX && e.position.x < (float) kStarsX + span && onRate)
            onRate(paint::starsAt(e.position.x, (float) kStarsX));
    }

private:
    juce::TextButton tag_, collect_, clear_, trash_;
    int count_ = 0;
};

}
