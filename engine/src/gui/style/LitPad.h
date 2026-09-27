// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <optional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/Contrast.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class LitPad : public juce::Button {
public:
    static constexpr float kPadSide = 34.0f;

    struct Look {
        std::optional<IconGlyph> glyph;
        juce::String text;
    };

    explicit LitPad(Look look) : juce::Button({}), look_(std::move(look)) { setClickingTogglesState(true); }

    static juce::Colour surface() { return Palette::panel; }

    static juce::Colour lamp(juce::Colour accent) {
        return contrast::isLight(surface()) ? accent.withMultipliedSaturation(1.12f).darker(0.12f) : accent;
    }
    static juce::Colour unlit(juce::Colour accent) {
        const auto base = surface();
        const auto tint = accent.withMultipliedSaturation(0.8f);
        return base.interpolatedWith(tint, 0.3f).darker(contrast::isLight(base) ? 0.12f : 0.06f);
    }

    void setFamily(int family) { family_ = family; repaint(); }
    void setWorking(bool working) { if (working != working_) { working_ = working; repaint(); } }
    void setLitOverride(juce::Colour c) { litOverride_ = c; repaint(); }
    void setUnlitOverride(juce::Colour c) { unlitOverride_ = c; repaint(); }

    juce::Colour litColour() const {
        if (working_) return lamp(Palette::warnAmber());
        if (litOverride_) return lamp(*litOverride_);
        return lamp(family_ >= 0 ? Palette::familyAccent((Family) family_) : Palette::accent);
    }
    juce::Colour unlitColour() const {
        if (unlitOverride_) return *unlitOverride_;
        return unlit(family_ >= 0 ? Palette::familyAccent((Family) family_) : Palette::accent);
    }
    juce::Colour capColour(bool on) const { return on ? litColour() : unlitColour(); }
    juce::Colour inkColour(bool on) const {
        const auto cap = capColour(on);
        return contrast::lifted(cap, contrast::readable(cap, ink::lit::shadow, litColour().brighter(0.3f)));
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const bool on = getToggleState();
        const auto full = getLocalBounds().toFloat();
        const float side = std::min({full.getWidth(), full.getHeight(), kPadSide});
        const bool wordy = !look_.glyph && look_.text.isNotEmpty();
        const auto cap = full.withSizeKeepingCentre(wordy ? full.getWidth() : side, side)
                             .reduced(1.0f).translated(0.0f, down ? 1.0f : 0.0f);
        const float r = side * 0.1f;
        const auto lit = litColour();

        if (on) {
            g.setColour(lit.withAlpha(alpha::muted));
            g.fillRoundedRectangle(cap.expanded(3.0f), r + 3.0f);
        }
        auto face = capColour(on);
        if (over) face = face.brighter(0.06f);
        g.setGradientFill(juce::ColourGradient(face.brighter(on ? 0.35f : 0.12f), cap.getX(), cap.getY(),
                                               face.darker(on ? 0.0f : 0.18f), cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle(cap, r * 0.7f);
        g.setColour(juce::Colours::white.withAlpha(contrast::isLight(face) ? alpha::veil : (on ? alpha::scrim : alpha::wash)));
        g.fillRoundedRectangle(cap.withHeight(cap.getHeight() * 0.42f).reduced(1.5f, 1.0f), r * 0.6f);
        g.setColour(ink::lit::shadow.withAlpha(contrast::isLight(face) ? alpha::muted : (on ? alpha::veil : alpha::dim)));
        g.drawRoundedRectangle(cap, r * 0.7f, 1.0f);

        const auto mark = inkColour(on);
        if (look_.glyph) {
            drawIconGlyph(g, *look_.glyph, cap.reduced(side * 0.2f), mark, true);
            return;
        }
        if (look_.text.isEmpty()) return;
        g.setColour(mark);
        g.setFont(juce::FontOptions(side * 0.34f, juce::Font::bold));
        g.drawText(look_.text, cap, juce::Justification::centred, false);
    }

private:
    Look look_;
    int family_ = -1;
    bool working_ = false;
    std::optional<juce::Colour> litOverride_, unlitOverride_;
};

}
