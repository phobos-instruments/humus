// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/BrandPanel.h"
#include "gui/common/Localisation.h"
#include "gui/help/GuideBody.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class WhatsNewWindow : public juce::Component {
public:
    static constexpr int kW = 640, kHeaderH = 64, kContentH = 460, kFooterH = 54;

    WhatsNewWindow(const std::string& markdown, const std::string& version, std::function<void()> onDismiss)
        : version_(juce::String(version)), onDismiss_(std::move(onDismiss)),
          body_(juce::String::fromUTF8(markdown.c_str())) {
        okBtn_.setColour(juce::TextButton::buttonColourId, ink::brand::line);
        okBtn_.setColour(juce::TextButton::textColourOffId, ink::brand::ground);
        okBtn_.onClick = [this] { dismiss(); };
        addAndMakeVisible(okBtn_);
        viewport_.setViewedComponent(&body_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);
        setWantsKeyboardFocus(true);
        setSize(kW, kHeaderH + kContentH + kFooterH);
    }

    void resized() override {
        viewport_.setBounds(juce::Rectangle<int>(0, kHeaderH, getWidth(), kContentH).reduced(12, 6));
        body_.layoutTo(viewport_.getMaximumVisibleWidth(), 0);
        okBtn_.setBounds(getLocalBounds().removeFromBottom(kFooterH).reduced(24, 11).removeFromRight(110));
    }

    void paint(juce::Graphics& g) override {
        paintBrandPanel(g, getLocalBounds(), ink::brand::ground, ink::brand::line);
        g.setColour(Palette::background);
        g.fillRect(juce::Rectangle<int>(0, kHeaderH, getWidth(), kContentH));
        drawHumusLogo(g, juce::Rectangle<float>(24.0f, 8.0f, 110.0f, (float) kHeaderH - 16.0f));
        g.setColour(ink::brand::line);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText(tr("whatsnew.title", "What's New"), 150, 0, 240, kHeaderH, juce::Justification::centredLeft);
        g.setColour(ink::brand::line.withAlpha(alpha::strong));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText("Humus " + version_, 0, 0, getWidth() - 24, kHeaderH, juce::Justification::centredRight);
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (k.getKeyCode() == juce::KeyPress::escapeKey || k.getKeyCode() == juce::KeyPress::returnKey) {
            dismiss();
            return true;
        }
        return false;
    }

private:
    void dismiss() {
        if (onDismiss_) onDismiss_();
    }

    juce::String version_;
    std::function<void()> onDismiss_;
    GuideBody body_;
    juce::Viewport viewport_;
    juce::TextButton okBtn_{tr("whatsnew.ok", "OK")};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WhatsNewWindow)
};

}
