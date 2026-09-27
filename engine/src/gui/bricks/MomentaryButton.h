// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <optional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/MomentaryPress.h"
#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class MomentaryButton : public juce::TextButton, private juce::Timer {
public:
    MomentaryButton(std::string param, const juce::String& caption, bool holdable = false,
                    bool confirmHold = false)
        : juce::TextButton(caption), param_(std::move(param)), press_(holdable, confirmHold) {
        setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        setColour(juce::TextButton::textColourOffId, Palette::text);
        setColour(juce::TextButton::textColourOnId, Palette::background);
        setTriggeredOnMouseDown(true);
        if (press_.firesOnClick()) onClick = [this] {
            press_.click();
            syncTimer();
        };
    }
    ~MomentaryButton() override { stopTimer(); }

    static constexpr float kIconScale = 0.5f;

    void setIcon(IconGlyph glyph) {
        icon_ = glyph;
        setTooltip(getButtonText());
        repaint();
    }
    std::optional<IconGlyph> icon() const { return icon_; }

    bool confirmHold() const { return press_.confirmHold(); }

    static constexpr float kDimAlpha = 0.35f;
    const std::string& param() const { return param_; }

    void setWriter(std::function<void(double)> write, std::function<int()> coverTicks) {
        press_.write = std::move(write);
        press_.coverTicks = std::move(coverTicks);
    }

    void showLamp(bool on, juce::Colour tint, bool dim) {
        if (findColour(buttonOnColourId) != tint) setColour(buttonOnColourId, tint);
        if (getToggleState() != on) setToggleState(on, juce::dontSendNotification);
        if (isEnabled() == dim) {
            setEnabled(!dim);
            setAlpha(dim ? kDimAlpha : 1.0f);
        }
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        if (!icon_) {
            juce::TextButton::paintButton(g, over, down);
            paintConfirm(g);
            return;
        }
        const bool on = getToggleState();
        getLookAndFeel().drawButtonBackground(g, *this, findColour(on ? buttonOnColourId : buttonColourId),
                                              over, down);
        const float side = juce::jmin((float) getWidth(), (float) getHeight()) * kIconScale;
        const auto fg = findColour(on ? textColourOnId : textColourOffId);
        g.setColour(fg);
        drawIconGlyph(g, *icon_, getLocalBounds().toFloat().withSizeKeepingCentre(side, side),
                      fg, isEnabled(), on);
        paintConfirm(g);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        juce::TextButton::mouseDown(e);
        press_.down(e.mods.isPopupMenu());
        syncTimer();
    }
    void mouseUp(const juce::MouseEvent& e) override {
        juce::TextButton::mouseUp(e);
        const bool wasConfirming = press_.confirming();
        press_.up(e.mods.isPopupMenu());
        syncTimer();
        if (wasConfirming) repaint();
    }

protected:
    void paintConfirm(juce::Graphics& g) const {
        if (!press_.confirming()) return;
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::accent.withAlpha(alpha::muted));
        g.fillRoundedRectangle(r.withWidth(r.getWidth() * (float) press_.confirmFraction()), 4.0f);
    }

private:
    void syncTimer() {
        if (press_.takeRestart()) startTimerHz(momentary::kTicksPerSecond);
        else if (!press_.running()) stopTimer();
    }

    void timerCallback() override {
        const bool wasConfirming = press_.confirming();
        press_.tick();
        syncTimer();
        if (wasConfirming) repaint();
    }

    std::string param_;
    momentary::Press press_;
    std::optional<IconGlyph> icon_;
};

}
