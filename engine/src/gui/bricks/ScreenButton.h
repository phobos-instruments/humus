// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/common/Localisation.h"

namespace hum {

class ScreenGlyphButton : public juce::Button {
public:
    ScreenGlyphButton() : juce::Button(tr("screen-button.open-the-video-window-2", "Open the video window")) {
        setTooltip(tr("screen-button.open-the-video-window", "Open the video window"));
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel.brighter(down ? 0.20f : over ? 0.12f : 0.05f));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        const auto tint = isEnabled() ? (over ? Palette::accent : Palette::text)
                                      : Palette::textDim.withAlpha(alpha::dim);
        auto box = r.reduced(r.getWidth() * 0.22f, r.getHeight() * 0.22f);
        const float standH = box.getHeight() * 0.22f;
        auto screen = box.withTrimmedBottom(standH);
        g.setColour(tint);
        g.drawRoundedRectangle(screen, 1.5f, 1.4f);
        const float cx = box.getCentreX();
        g.fillRect(cx - 0.7f, screen.getBottom(), 1.4f, standH * 0.55f);
        g.fillRect(cx - box.getWidth() * 0.22f, box.getBottom() - 1.4f, box.getWidth() * 0.44f,
                   1.4f);
    }
};

class ScreenButton : public OrganismEditor {
public:
    ScreenButton(BrickHost& host, std::string organism)
        : host_(host), name_(std::move(organism)) {
        addAndMakeVisible(button_);
        button_.onClick = [this] {
            host_.showVisuals(name_);
        };
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 22; }
    int preferredContentHeight(int) const override { return 22; }
    void resized() override { button_.setBounds(getLocalBounds()); }

private:
    BrickHost& host_;
    std::string name_;
    ScreenGlyphButton button_;
};

}
