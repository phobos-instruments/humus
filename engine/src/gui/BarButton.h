#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/IconGlyph.h"
#include "gui/LookAndFeel.h"

namespace hum {

class BarButton : public juce::Button {
public:
    BarButton(IconGlyph g, juce::String caption, juce::String tooltip)
        : juce::Button(caption), glyph_(g), caption_(std::move(caption)) {
        setTooltip(tooltip);
    }

    void setOn(bool b) { if (b != on_) { on_ = b; repaint(); } }
    bool isOn() const { return on_; }
    void setOnColour(juce::Colour c) { onColour_ = c; repaint(); }

    void setCompact(bool c) { if (c != compact_) { compact_ = c; repaint(); } }

    std::function<void(juce::Point<int>)> onRightClick;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu() && onRightClick) {
            grabKeyboardFocus();
            onRightClick(e.getScreenPosition());
            return;
        }
        juce::Button::mouseDown(e);
    }

    int widthFor(bool compact) const {
        if (compact) return kGlyph + 2 * kPad;
        return kGlyph + kGap + captionWidth() + kSlack + 2 * kPad;
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto b = getLocalBounds().toFloat().reduced(0.5f);
        const bool enabled = isEnabled();
        const juce::Colour fill =
            !enabled            ? Palette::panel
            : on_               ? onColour_
            : down              ? Palette::accentDim
            : over              ? Palette::panelLight
                                : Palette::panel;
        g.setColour(fill);
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(!enabled ? Palette::border.withAlpha(0.5f)
                             : on_ ? onColour_.brighter(0.3f)
                             : over ? Palette::accent.withAlpha(0.5f)
                                    : Palette::border);
        g.drawRoundedRectangle(b, 4.0f, 1.0f);

        const juce::Colour ink = !enabled ? Palette::border
                                          : on_ ? juce::Colours::black
                                                : Palette::textDim;
        auto inner = b.reduced((float) kPad, 0.0f);
        auto gr = inner.removeFromLeft((float) kGlyph);
        drawIconGlyph(g, glyph_, gr.withSizeKeepingCentre((float) kGlyph, (float) kGlyph),
                      ink, enabled);
        if (!compact_) {
            inner.removeFromLeft((float) kGap);
            g.setColour(ink);
            g.setFont(captionFont());
            g.drawText(caption_, inner, juce::Justification::centredLeft, false);
        }
    }

private:
    static constexpr int kGlyph = 13;
    static constexpr int kGap = 5;
    static constexpr int kPad = 6;
    static constexpr int kSlack = 4;

    static juce::Font captionFont() {
        return juce::Font(juce::FontOptions(10.5f).withStyle("Bold"));
    }
    int captionWidth() const {
        return juce::GlyphArrangement::getStringWidthInt(captionFont(), caption_);
    }

    IconGlyph glyph_;
    juce::String caption_;
    juce::Colour onColour_ = Palette::recordRed();
    bool on_ = false;
    bool compact_ = false;
};

}
