// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/ChoiceModel.h"
#include "gui/editor/LayoutModel.h"
#include "gui/editor/Mappable.h"
#include "gui/editor/juce/JuceControlArt.h"
#include "gui/editor/juce/JuceView.h"
#include "gui/editor/views/ChoiceView.h"
#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"

namespace hum {

class WaveGlyphButton : public juce::Button {
public:
    WaveGlyphButton(const juce::String& name, WaveGlyph g, juce::Colour lit)
        : juce::Button(name), glyph_(g), lit_(lit) {}

    juce::String glyphName() const { return waveGlyphDescription(glyph_); }

    void paintButton(juce::Graphics& g, bool over, bool) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(getToggleState() ? lit_
                    : over            ? Palette::panelLight.brighter(0.15f)
                                      : Palette::panelLight);
        g.fillRect(r);
        g.setColour(Palette::border);
        g.drawRect(r, 1.0f);
        drawWaveGlyph(g, glyph_, r, getToggleState() ? Palette::background : Palette::textDim);
    }

private:
    WaveGlyph glyph_;
    juce::Colour lit_;
};

class ArtSegment : public juce::TextButton {
public:
    ArtSegment(const juce::String& name, std::shared_ptr<const ButtonArt> art, std::optional<WaveGlyph> glyph)
        : juce::TextButton(name), art_(std::move(art)), glyph_(glyph) {}

    void paintButton(juce::Graphics& g, bool, bool down) override {
        art_->paintPlate(g, *this, down);
        if (!glyph_) {
            art_->paintCaption(g, *this, std::nullopt);
            return;
        }
        if (art_->caption())
            drawWaveGlyph(g, *glyph_, getLocalBounds().toFloat(), art_->text().withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));
    }

private:
    std::shared_ptr<const ButtonArt> art_;
    std::optional<WaveGlyph> glyph_;
};

class JuceChoiceButtons : public JuceGroupView<ChoiceButtonsView> {
public:
    JuceChoiceButtons(juce::Component& parent, const choice::Setup& setup, int family) {
        const auto lit = Palette::familyAccent((Family) family);
        const auto art = loadedArt(setup.art);
        for (size_t i = 0; i < setup.options.size(); ++i) {
            const auto label = viewText(setup.options[i].label);
            std::unique_ptr<juce::Button> btn;
            auto menu = [this](juce::Point<int> at) { if (onMenu) onMenu(pointOf(at)); };
            if (art) {
                auto glyph = setup.waveIcons ? std::optional<WaveGlyph>(waveGlyphFor(label)) : std::nullopt;
                auto ab = std::make_unique<Mappable<ArtSegment>>(label, art, glyph);
                if (glyph) ab->setTooltip(waveGlyphDescription(*glyph));
                ab->onRightClick = menu;
                btn = std::move(ab);
            } else if (setup.waveIcons) {
                auto wb = std::make_unique<Mappable<WaveGlyphButton>>(label, waveGlyphFor(label), lit);
                wb->setTooltip(wb->glyphName());
                wb->onRightClick = menu;
                btn = std::move(wb);
            } else {
                auto tb = std::make_unique<Mappable<juce::TextButton>>(label);
                tb->onRestyle = [b = tb.get(), family] {
                    b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
                    b->setColour(juce::TextButton::buttonOnColourId, Palette::familyAccent((Family) family));
                    b->setColour(juce::TextButton::textColourOffId, Palette::textDim);
                    b->setColour(juce::TextButton::textColourOnId, Palette::background);
                };
                tb->onRestyle();
                tb->setConnectedEdges((i > 0 ? juce::Button::ConnectedOnLeft : 0)
                                      | (i + 1 < setup.options.size() ? juce::Button::ConnectedOnRight : 0));
                tb->onRightClick = menu;
                btn = std::move(tb);
            }
            if (const auto tip = choice::tipForIndex(setup, (int) i); !tip.empty())
                btn->setTooltip(viewText(tip));
            btn->onClick = [this, i] { if (onChoose) onChoose((int) i); };
            parent.addAndMakeVisible(*btn);
            parts_.push_back(btn.get());
            buttons_.push_back(std::move(btn));
        }
    }

    void setViewBounds(Rect bounds) override {
        auto row = toJuce(bounds);
        const int w = layout::radioCellWidth(row.getWidth(), (int) buttons_.size());
        for (size_t j = 0; j < buttons_.size(); ++j)
            buttons_[j]->setBounds(j + 1 == buttons_.size() ? row : row.removeFromLeft(w));
    }
    void setViewTooltip(const std::string& tip) override {
        for (auto& b : buttons_) b->setTooltip(viewText(tip));
    }
    void showIndex(int index) override {
        for (size_t j = 0; j < buttons_.size(); ++j)
            buttons_[j]->setToggleState((int) j == index, juce::dontSendNotification);
    }
    int optionCount() const override { return (int) buttons_.size(); }
    int shownIndex() const override {
        for (size_t j = 0; j < buttons_.size(); ++j)
            if (buttons_[j]->getToggleState()) return (int) j;
        return -1;
    }

private:
    static std::shared_ptr<const ButtonArt> loadedArt(const ControlArt& art) {
        if (art.empty()) return nullptr;
        auto loaded = std::make_shared<const ButtonArt>(art);
        return loaded->valid() ? loaded : nullptr;
    }

    std::vector<std::unique_ptr<juce::Button>> buttons_;
};

class ListArrow : public juce::Button {
public:
    explicit ListArrow(bool forward) : juce::Button({}), fwd_(forward) { setRepeatSpeed(400, 140); }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const auto r = getLocalBounds().toFloat();
        const float w = 6.0f, h = 8.0f;
        const float x = r.getCentreX() - w * 0.5f, y = r.getCentreY() - h * 0.5f;
        juce::Path p;
        if (fwd_) p.addTriangle(x, y, x, y + h, x + w, y + h * 0.5f);
        else      p.addTriangle(x + w, y, x + w, y + h, x, y + h * 0.5f);
        g.setColour(!isEnabled() ? Palette::border.brighter(0.08f)
                    : (down || over) ? Palette::text : Palette::textDim);
        g.fillPath(p);
    }

private:
    bool fwd_;
};

class OpeningCombo : public juce::ComboBox {
public:
    std::function<void()> beforeOpen;

    void setArt(const ControlArt& art) {
        if (art.empty()) return;
        ComboArt loaded(art);
        if (!loaded.valid()) return;
        setColour(juce::ComboBox::textColourId, loaded.text());
        art_.emplace(std::move(loaded));
    }

    void lookAndFeelChanged() override {
        juce::ComboBox::lookAndFeelChanged();
        if (!art_) setColour(juce::ComboBox::textColourId, Palette::text);
    }

    void paint(juce::Graphics& g) override {
        if (art_) art_->paint(g, *this);
        else juce::ComboBox::paint(g);
    }

    void showPopup() override {
        if (beforeOpen) beforeOpen();
        juce::ComboBox::showPopup();
    }

private:
    std::optional<ComboArt> art_;
};

class JuceComboView : public JuceGroupView<ComboView> {
public:
    JuceComboView(juce::Component& parent, bool steppers, const ControlArt& art = {}) {
        combo_.setColour(juce::ComboBox::textColourId, Palette::text);
        combo_.setArt(art);
        combo_.onChange = [this] { if (onPick) onPick(combo_.getSelectedId()); };
        combo_.beforeOpen = [this] { if (onOpen) onOpen(); };
        parent.addAndMakeVisible(combo_);
        parts_.push_back(&combo_);
        if (!steppers) return;
        for (bool forward : {false, true}) {
            auto arrow = std::make_unique<ListArrow>(forward);
            arrow->onClick = [this, forward] { if (onStep) onStep(forward); };
            parent.addAndMakeVisible(*arrow);
            parts_.push_back(arrow.get());
            arrows_.push_back(std::move(arrow));
        }
    }

    void setViewBounds(Rect bounds) override {
        auto r = toJuce(bounds);
        if (arrows_.size() == 2) {
            const int a = layout::stepperArrowWidth(r.getWidth());
            arrows_[0]->setBounds(r.removeFromLeft(a));
            arrows_[1]->setBounds(r.removeFromRight(a));
        }
        combo_.setBounds(r);
    }
    void setViewTooltip(const std::string& tip) override {
        for (auto* part : parts_)
            if (auto* client = dynamic_cast<juce::SettableTooltipClient*>(part)) client->setTooltip(viewText(tip));
    }
    void showTip(const std::string& tip) override { combo_.setTooltip(viewText(tip)); }

    void setItems(const std::vector<ComboItem>& items) override {
        combo_.clear(juce::dontSendNotification);
        for (const auto& item : items) combo_.addItem(viewText(item.text), item.id);
    }
    void showSelectedId(int id) override { combo_.setSelectedId(id, juce::dontSendNotification); }
    int shownSelectedId() const override { return combo_.getSelectedId(); }
    std::string shownText() const override { return combo_.getText().toStdString(); }

    juce::ComboBox& comboBox() { return combo_; }

private:
    OpeningCombo combo_;
    std::vector<std::unique_ptr<ListArrow>> arrows_;
};

}
