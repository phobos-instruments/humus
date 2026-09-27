// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>
#include <initializer_list>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/CardStack.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class TimedCard : public juce::Component {
public:
    static constexpr int kTickHz = 30;
    static constexpr int kCardWidth = 312;
    static constexpr float kRadius = 12.0f;
    static constexpr int kPad = 14;
    static constexpr int kButtonGap = 8;
    static constexpr int kButtonPadX = 12;
    static constexpr int kButtonH = 26;
    static constexpr int kButtonBottom = 14;
    static constexpr float kBarH = 3.0f;

    std::function<void()> onDismiss;

    TimedCard() {
        dismiss_.setButtonText(tr("cards.ok", "OK"));
        dismiss_.onClick = [this] { dismiss(); };
        addAndMakeVisible(dismiss_);
    }

    static void paintWorkingBar(juce::Graphics& g, juce::Rectangle<float> bar, int ticks) {
        g.setColour(Palette::panelLight);
        g.fillRoundedRectangle(bar, 2.0f);
        const float span = bar.getWidth() * 0.3f;
        const float phase = (float) (ticks % (2 * kTickHz)) / (float) (2 * kTickHz);
        const float x = bar.getX() + (bar.getWidth() + span) * phase - span;
        g.setColour(Palette::accent);
        g.fillRoundedRectangle(bar.withX(x).withWidth(span).getIntersection(bar), 2.0f);
    }

    void expireIn(double seconds) {
        total_ = std::max(1, (int) (seconds * kTickHz));
        left_ = total_;
        ticker_.startTimerHz(kTickHz);
        repaint();
    }

    void keep() {
        ticker_.stopTimer();
        total_ = left_ = 0;
        repaint();
    }

    bool expires() const { return total_ > 0; }
    bool dismissed() const { return dismissed_; }
    float timeLeft() const { return total_ > 0 ? (float) left_ / (float) total_ : 0.0f; }
    juce::TextButton& dismissButton() { return dismiss_; }

    void dismiss() {
        keep();
        dismissed_ = true;
        if (auto* stack = findParentComponentOfClass<CardStack>()) { stack->remove(this); return; }
        if (onDismiss) onDismiss();
    }

    void resized() override {
        if (dismiss_.isVisible()) dismiss_.setBounds(buttonRow().removeFromRight(buttonWidth(dismiss_, kButtonH)));
        layout();
    }

protected:
    virtual void layout() {}
    virtual bool holdsTimer() const { return isMouseOver(true); }

    int titleWidth() const { return getWidth() - 2 * kPad; }

    void hideDismiss() { dismiss_.setVisible(false); }

    static int buttonWidth(juce::TextButton& b, int height) {
        return b.getBestWidthForHeight(height) - height + 2 * kButtonPadX;
    }

    void paintBody(juce::Graphics& g, bool lit) const {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, kRadius);
        if (expires()) {
            const juce::Graphics::ScopedSaveState clip(g);
            juce::Path shape;
            shape.addRoundedRectangle(r, kRadius);
            g.reduceClipRegion(shape);
            g.setColour((held_ ? Palette::textDim : Palette::accent).withAlpha(alpha::strong));
            g.fillRect(r.withTop(r.getBottom() - kBarH).withWidth(r.getWidth() * timeLeft()));
        }
        g.setColour(lit ? Palette::accent.withAlpha(alpha::strong) : Palette::border);
        g.drawRoundedRectangle(r, kRadius, 1.0f);
    }

    static void placeButtons(juce::Rectangle<int> row, std::initializer_list<juce::TextButton*> buttons) {
        const int h = row.getHeight();
        for (auto* b : buttons) {
            if (!b->isVisible()) continue;
            b->setBounds(row.removeFromLeft(buttonWidth(*b, h)));
            row.removeFromLeft(kButtonGap);
        }
    }

    juce::Rectangle<int> buttonRow(int height = kButtonH) const {
        return getLocalBounds().reduced(kPad - 2, kButtonBottom).removeFromBottom(height);
    }

private:
    void tick() {
        const bool held = holdsTimer();
        if (held != held_) {
            held_ = held;
            repaint();
        }
        if (held) return;
        if (--left_ > 0) {
            repaint();
            return;
        }
        dismiss();
    }

    struct Ticker : juce::Timer {
        explicit Ticker(TimedCard& c) : card(c) {}
        void timerCallback() override { card.tick(); }
        TimedCard& card;
    };

    Ticker ticker_{*this};
    juce::TextButton dismiss_;
    int total_ = 0, left_ = 0;
    bool held_ = false;
    bool dismissed_ = false;
};

}
