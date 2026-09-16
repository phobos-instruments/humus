// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/Categories.h"
#include "gui/style/Colours.h"

namespace hum {

namespace Palette {
    inline juce::Colour background {0xff171410};
    inline juce::Colour panel      {0xff231e16};
    inline juce::Colour panelLight {0xff2f2920};
    inline juce::Colour border     {0xff473e2e};
    inline juce::Colour accent     {0xff9fbf5a};
    inline juce::Colour accentDim  {0xff5c6e34};
    inline juce::Colour text       {0xffefe6d2};
    inline juce::Colour textDim    {0xffa39a83};
    inline juce::Colour cord       {0xff7a8a4e};
    inline juce::Colour derivedMidiCord = ink::unset;
    inline juce::Colour derivedVideoCord = ink::unset;
    inline juce::Colour derivedFamily[5] = {ink::unset, ink::unset, ink::unset,
                                            ink::unset, ink::unset};
    inline juce::Colour derivedFamilyDim[5] = {ink::unset, ink::unset, ink::unset,
                                               ink::unset, ink::unset};

    inline juce::Colour midiCord() { return derivedMidiCord; }
    inline juce::Colour videoCord() { return derivedVideoCord; }
    inline juce::Colour controlCord() { return ink::state::controlCord; }
    inline juce::Colour recordRed() { return ink::state::recording; }
    inline juce::Colour warnAmber() { return ink::state::warning; }

    inline juce::Colour familyAccent(Family f) { return derivedFamily[(int) f]; }
    inline juce::Colour familyAccentDim(Family f) { return derivedFamilyDim[(int) f]; }
}

namespace foxfire {
    inline void bloom(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c) {
        const auto centre = b.getCentre().translated(0.0f, b.getHeight() * 0.10f);
        const float rad = juce::jmin(b.getWidth(), b.getHeight()) * 0.42f;
        juce::ColourGradient gr(c.withAlpha(alpha::muted), centre.x, centre.y,
                                c.withAlpha(alpha::none), centre.x, centre.y + rad, true);
        gr.addColour(0.55, c.withAlpha(alpha::mist));
        g.setGradientFill(gr);
        g.fillEllipse(juce::Rectangle<float>(rad * 2.0f, rad * 2.0f).withCentre(centre));
    }
}

struct ThemeColours {
    juce::Colour background, panel, panelLight, border, accent, accentDim, text, textDim, cord;
};

int numThemes();
const char* themeName(int index);
ThemeColours presetColours(int index);

void applyTheme(int index);
void setBaseColours(const ThemeColours&);
ThemeColours baseColours();

void setAppearanceAdjustments(double brightness, double contrast);
double brightnessAdjustment();
double contrastAdjustment();
void recomputePalette();

class HumLookAndFeel : public juce::LookAndFeel_V4 {
public:
    HumLookAndFeel();
    ~HumLookAndFeel() override;

    void refreshColours();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float pos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    int getSliderThumbRadius(juce::Slider&) override { return 9; }

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& background,
                              bool highlighted, bool down) override;
    void drawTickBox(juce::Graphics&, juce::Component&, float x, float y, float w, float h,
                     bool ticked, bool enabled, bool highlighted, bool down) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics&,
                                  const juce::Path&, juce::Image&) override;
    void drawProgressBar(juce::Graphics&, juce::ProgressBar&, int width, int height,
                         double progress, const juce::String& textToShow) override;
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h,
                       bool isVertical, int thumbStart, int thumbSize,
                       bool mouseOver, bool mouseDown) override;
    bool areScrollbarButtonsVisible() override { return false; }
};

void pebbleBody(juce::Graphics&, juce::Rectangle<float> bounds, juce::Colour base, bool active);

void fillOrganicPill(juce::Graphics&, juce::Rectangle<float>, float cornerRadius,
                     juce::Colour fill, juce::Colour outline, bool raised);

void paintVerticalFader(juce::Graphics&, juce::Rectangle<float> bounds, float thumbY, bool muted);
void paintHorizontalFader(juce::Graphics&, juce::Rectangle<float> bounds, float thumbX, bool muted);

void drawFaderPot(juce::Graphics&, juce::Rectangle<float> body,
                  float gradTop, float gradBottom, float lineY, bool active,
                  bool roundTop, bool roundBottom, juce::Colour lineColour);

void paintFaderGroove(juce::Graphics&, juce::Rectangle<float> bounds,
                      float fillLo, float fillHi, bool muted, bool vertical = true);

}
