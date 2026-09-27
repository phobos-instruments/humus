// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <memory>
#include <optional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/LooperBricks.h"
#include "gui/bricks/StrandColours.h"
#include "gui/editor/Mappable.h"
#include "gui/editor/MomentaryModel.h"
#include "gui/editor/ToggleModel.h"
#include "gui/editor/juce/JuceControlArt.h"
#include "gui/editor/juce/JuceView.h"
#include "gui/editor/views/BrickView.h"
#include "gui/editor/views/ToggleView.h"
#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LitPad.h"

namespace hum {

class JuceLabelView : public JuceView<juce::Label, LabelView> {
public:
    explicit JuceLabelView(LabelKind kind) : kind_(kind) {}

    void showText(const std::string& text) override {
        setText(viewText(text), juce::dontSendNotification);
    }
    int textWidth() const override {
        return (int) std::ceil(juce::TextLayout::getStringWidth(getFont(), getText()));
    }

    void lookAndFeelChanged() override {
        juce::Label::lookAndFeelChanged();
        inkForKind();
    }

    void inkForKind() {
        const bool plainText = kind_ == LabelKind::Plain;
        setColour(juce::Label::textColourId, plainText ? Palette::text : Palette::textDim);
    }

    void paint(juce::Graphics& g) override {
        if (kind_ == LabelKind::Section) {
            paintSection(g);
            return;
        }
        if (kind_ == LabelKind::Pill) {
            const auto r = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
        }
        juce::Label::paint(g);
    }

private:
    void paintSection(juce::Graphics& g) {
        const float inset = kSectionTitleHeight * 0.5f;
        const auto frame = getLocalBounds().toFloat().reduced(0.5f).withTrimmedTop(inset);
        g.setColour(Palette::border.withAlpha(alpha::mid));
        g.drawRoundedRectangle(frame, 6.0f, 1.0f);
        const auto font = getFont();
        const float textW = juce::TextLayout::getStringWidth(font, getText()) + 8.0f;
        const juce::Rectangle<float> title(frame.getX() + 10.0f, 0.0f, textW, kSectionTitleHeight);
        g.setColour(Palette::panel);
        g.fillRect(title);
        g.setColour(Palette::textDim);
        g.setFont(font);
        g.drawText(getText(), title, juce::Justification::centred, false);
    }

    static constexpr float kSectionTitleHeight = 14.0f;
    LabelKind kind_;
};

inline std::unique_ptr<JuceLabelView> makeJuceLabel(LabelKind kind, const std::string& text) {
    auto view = std::make_unique<JuceLabelView>(kind);
    view->showText(text);
    const bool pill = kind == LabelKind::Pill;
    if (kind == LabelKind::Section) {
        view->setFont(juce::FontOptions(10.5f));
        view->setInterceptsMouseClicks(false, false);
        return view;
    }
    view->inkForKind();
    if (kind == LabelKind::Plain || pill) {
        view->setJustificationType(pill ? juce::Justification::centred
                                        : juce::Justification::centredLeft);
        view->setFont(juce::FontOptions(pill ? 12.0f : 11.0f));
        return view;
    }
    view->setJustificationType(kind == LabelKind::Above ? juce::Justification::centred
                                                        : juce::Justification::centredRight);
    view->setFont(juce::FontOptions(10.0f));
    view->setMinimumHorizontalScale(0.6f);
    return view;
}

template <class Widget>
class JuceToggleView : public JuceView<Mappable<Widget>, ToggleView> {
public:
    using JuceView<Mappable<Widget>, ToggleView>::JuceView;

    void showMarks(bool externallyControlled, bool rollLocked) override {
        this->setMarks(externallyControlled, rollLocked);
    }

    void setArt(const ControlArt& art) {
        if (art.empty()) return;
        if (ButtonArt loaded(art); loaded.valid()) art_.emplace(std::move(loaded));
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        if (!art_) {
            Widget::paintButton(g, over, down);
            return;
        }
        art_->paintPlate(g, *this, down);
        art_->paintCaption(g, *this, std::nullopt);
    }

    void showOn(bool on) override {
        this->setToggleState(on, juce::dontSendNotification);
        if (this->onStateChange) this->onStateChange();
    }
    bool shownOn() const override { return this->getToggleState(); }

    void wire() {
        this->onClick = [this] { if (onToggle) onToggle(this->getToggleState()); };
        this->onRightClick = [this](juce::Point<int> at) { if (onMenu) onMenu(pointOf(at)); };
    }

    using ToggleView::onMenu;
    using ToggleView::onToggle;

private:
    std::optional<ButtonArt> art_;
};

inline std::unique_ptr<ToggleView> makeJuceToggle(const toggle::Setup& setup) {
    if (setup.kind == toggle::Kind::Lit) {
        ControlArt onArt, offArt;
        onArt.text = setup.onColour;
        offArt.text = setup.offColour;
        const auto glyph = setup.icon == "none" ? std::nullopt : iconGlyphNamed(setup.icon);
        auto lit = std::make_unique<JuceToggleView<LitPad>>(
            LitPad::Look{glyph, glyph ? juce::String() : viewText(toggle::textOf(setup, false))});
        lit->setFamily(setup.family);
        if (!setup.onColour.empty()) lit->setLitOverride(artTextColour(onArt, Palette::accent));
        if (!setup.offColour.empty()) lit->setUnlitOverride(artTextColour(offArt, Palette::accent));
        lit->wire();
        return lit;
    }
    if (setup.kind == toggle::Kind::Tick) {
        auto tick = std::make_unique<JuceToggleView<juce::ToggleButton>>(
            viewText(toggle::textOf(setup, false)));
        tick->onRestyle = [t = tick.get()] { t->setColour(juce::ToggleButton::textColourId, Palette::text); };
        tick->onRestyle();
        tick->wire();
        return tick;
    }
    auto tb = std::make_unique<JuceToggleView<juce::TextButton>>(viewText(toggle::textOf(setup, false)));
    tb->setClickingTogglesState(true);
    tb->onRestyle = [b = tb.get()] {
        b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        b->setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        b->setColour(juce::TextButton::textColourOffId, Palette::textDim);
        b->setColour(juce::TextButton::textColourOnId, Palette::background);
    };
    tb->onRestyle();
    if (!setup.onLabel.empty()) {
        auto* p = tb.get();
        tb->onStateChange = [p, setup] {
            p->setButtonText(viewText(toggle::textOf(setup, p->getToggleState())));
        };
    }
    tb->setArt(setup.art);
    tb->wire();
    return tb;
}

class JuceMomentaryView : public JuceView<Mappable<MomentaryButton>, MomentaryView> {
public:
    explicit JuceMomentaryView(const momentary::Setup& setup)
        : JuceView<Mappable<MomentaryButton>, MomentaryView>(setup.param, viewText(setup.label),
                                                             setup.holdable, setup.confirmHold) {
        setWriter([this](double value) { if (onWrite) onWrite(value); },
                  [this] { return coverTicks ? coverTicks() : momentary::coverTicksFor(0.0, 0); });
        onRightClick = [this](juce::Point<int> at) { if (onMenu) onMenu(pointOf(at)); };
        if (const auto glyph = iconGlyphNamed(setup.icon)) setIcon(*glyph);
        if (!setup.art.empty())
            if (ButtonArt loaded(setup.art); loaded.valid()) art_.emplace(std::move(loaded));
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        if (!art_) {
            MomentaryButton::paintButton(g, over, down);
            return;
        }
        art_->paintPlate(g, *this, down);
        art_->paintCaption(g, *this, icon());
        paintConfirm(g);
    }

    void showMarks(bool externallyControlled, bool rollLocked) override {
        setMarks(externallyControlled, rollLocked);
    }

    void showHeld(bool held) override {
        if (getToggleState() != held) setToggleState(held, juce::dontSendNotification);
    }
    bool shownHeld() const override { return getToggleState(); }
    void showLamp(bool on, int tint, bool dim) override {
        MomentaryButton::showLamp(on, strandStateColour(tint), dim);
    }

    using MomentaryView::coverTicks;
    using MomentaryView::onMenu;
    using MomentaryView::onWrite;

private:
    std::optional<ButtonArt> art_;
};

}
