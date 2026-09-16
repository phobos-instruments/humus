// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "gui/app/AppSettings.h"
#include "gui/settings/HexColour.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class ColourSwatch : public juce::Component, private juce::ChangeListener {
public:
    static constexpr int kChipW = 30;
    static constexpr int kGap = 4;

    ColourSwatch() {
        hex_.setInputRestrictions(7, "#0123456789abcdefABCDEF");
        hex_.setJustification(juce::Justification::centredLeft);
        hex_.setSelectAllWhenFocused(true);
        hex_.setTooltip(tr("colour-swatch.hex", "Hex colour - Return to apply"));
        hex_.onReturnKey = [this] { commit(); };
        hex_.onFocusLost = [this] { commit(); };
        hex_.onEscapeKey = [this] { hex_.setText(hexText(colour_), false); };
        addAndMakeVisible(hex_);
        restyle();
        show(colour_);
    }

    ~ColourSwatch() override {
        if (selector_ != nullptr) selector_->removeChangeListener(this);
    }

    std::function<void(juce::Colour)> onChange;

    juce::Colour colour() const { return colour_; }

    void show(juce::Colour c) {
        colour_ = c;
        hex_.setText(hexText(c), false);
        repaint();
    }

    juce::TextEditor& hexField() { return hex_; }

    void paint(juce::Graphics& g) override {
        const auto chip = chipBounds();
        g.setColour(colour_);
        g.fillRect(chip);
        g.setColour(Palette::border);
        g.drawRect(chip, 1);
    }

    void resized() override {
        hex_.setBounds(getLocalBounds().withTrimmedLeft(kChipW + kGap));
    }

    void lookAndFeelChanged() override { restyle(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!chipBounds().contains(e.getPosition())) return;
        if (hex_.hasKeyboardFocus(false)) commit();
        struct BatchedSelector : juce::ColourSelector {
            using juce::ColourSelector::ColourSelector;
            ~BatchedSelector() override { AppSettings::instance().endBatch(); }
        };
        AppSettings::instance().beginBatch();
        auto sel = std::make_unique<BatchedSelector>(
            juce::ColourSelector::showColourAtTop | juce::ColourSelector::editableColour
            | juce::ColourSelector::showSliders | juce::ColourSelector::showColourspace);
        sel->setCurrentColour(colour_);
        sel->setSize(240, 280);
        sel->addChangeListener(this);
        selector_ = sel.get();
        juce::CallOutBox::launchAsynchronously(std::move(sel), localAreaToGlobal(chipBounds()),
                                               nullptr);
    }

private:
    juce::Rectangle<int> chipBounds() const { return getLocalBounds().withWidth(kChipW); }

    void restyle() {
        hex_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f,
                                       juce::Font::plain));
        hex_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
        hex_.setColour(juce::TextEditor::textColourId, Palette::text);
        hex_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        hex_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
        hex_.applyFontToAllText(hex_.getFont());
        hex_.applyColourToAllText(Palette::text);
    }

    void commit() {
        const auto parsed = parseHexColour(hex_.getText());
        if (!parsed || *parsed == colour_) {
            hex_.setText(hexText(colour_), false);
            return;
        }
        colour_ = *parsed;
        hex_.setText(hexText(colour_), false);
        repaint();
        if (onChange) onChange(colour_);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* src) override {
        if (auto* s = dynamic_cast<juce::ColourSelector*>(src)) {
            show(s->getCurrentColour());
            if (onChange) onChange(colour_);
        }
    }

    juce::Colour colour_{juce::Colours::black};
    juce::TextEditor hex_;
    juce::Component::SafePointer<juce::ColourSelector> selector_;
};

}
