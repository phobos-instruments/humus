// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

inline void paintChosen(juce::Graphics& g, juce::Rectangle<float> r) {
    const auto box = r.reduced(0.5f);
    g.setColour(Palette::accent.withAlpha(alpha::mist));
    g.fillRoundedRectangle(box, 4.0f);
    g.setColour(Palette::accent);
    g.drawRoundedRectangle(box, 4.0f, 1.4f);
}

class ModeChip : public juce::TextButton {
public:
    using Glyph = std::function<void(juce::Graphics&, juce::Rectangle<float>, juce::Colour)>;

    ModeChip(const juce::String& text, Glyph glyph) : juce::TextButton(text), glyph_(std::move(glyph)) {
        setClickingTogglesState(false);
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const bool on = getToggleState();
        getLookAndFeel().drawButtonBackground(
            g, *this, findColour(on ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId),
            over, down);
        if (on) paintChosen(g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced(5.0f, 3.0f);
        const auto ink = on ? Palette::accent : findColour(juce::TextButton::textColourOffId);
        if (glyph_) {
            glyph_(g, r.removeFromLeft(r.getHeight()), ink);
            r.removeFromLeft(4.0f);
        }
        g.setColour(ink);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(getButtonText(), r.toNearestInt(),
                         glyph_ ? juce::Justification::centredLeft : juce::Justification::centred, 1);
    }

private:
    Glyph glyph_;
};

template <typename E>
class ModeChips : public juce::Component {
public:
    std::function<void(E)> onPick;

    void setOptions(const std::vector<E>& values, const std::function<juce::String(E)>& text,
                    const std::function<void(juce::Graphics&, E, juce::Rectangle<float>, juce::Colour)>& glyph = nullptr) {
        values_ = values;
        chips_.clear();
        for (E v : values) {
            ModeChip::Glyph g;
            if (glyph) g = [glyph, v](juce::Graphics& gr, juce::Rectangle<float> r, juce::Colour c) { glyph(gr, v, r, c); };
            auto chip = std::make_unique<ModeChip>(text(v), std::move(g));
            chip->onClick = [this, v] {
                setSelected(v);
                if (onPick) onPick(v);
            };
            addAndMakeVisible(*chip);
            chips_.push_back(std::move(chip));
        }
        resized();
    }

    void setSelected(E v) {
        for (size_t i = 0; i < chips_.size(); ++i)
            chips_[i]->setToggleState(values_[i] == v, juce::dontSendNotification);
    }

    int count() const { return (int) chips_.size(); }
    juce::Button* chipForTest(int i) { return i >= 0 && i < count() ? chips_[(size_t) i].get() : nullptr; }

    void resized() override {
        if (chips_.empty()) return;
        auto r = getLocalBounds();
        const int gap = 3;
        const int w = (r.getWidth() - gap * ((int) chips_.size() - 1)) / (int) chips_.size();
        for (auto& c : chips_) {
            c->setBounds(r.removeFromLeft(w));
            r.removeFromLeft(gap);
        }
    }

private:
    std::vector<E> values_;
    std::vector<std::unique_ptr<ModeChip>> chips_;
};

}
