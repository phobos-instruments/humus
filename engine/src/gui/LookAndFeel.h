#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Categories.h"

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
    inline juce::Colour midiCord() { return cord.withRotatedHue(-0.22f).brighter(0.15f); }
    inline juce::Colour videoCord() { return cord.withRotatedHue(0.35f).brighter(0.25f); }
    inline juce::Colour recordRed() { return juce::Colour(0xffd6553f); }
    inline juce::Colour warnAmber() { return juce::Colour(0xffd9a03c); }

    inline juce::Colour familyAccent(Family f) {
        switch (f) {
            case Family::Voice:  return accent;
            case Family::Time:   return accent.withRotatedHue(0.225f).brighter(0.12f);
            case Family::Motion: return accent.withRotatedHue(-0.105f).brighter(0.04f);
            case Family::Sense:  return accent.withRotatedHue(0.525f).brighter(0.20f);
            case Family::Utility: break;
        }
        return textDim;
    }
    inline juce::Colour familyAccentDim(Family f) {
        if (f == Family::Utility) return textDim.darker(0.45f);
        return familyAccent(f).darker(0.72f);
    }
}

namespace foxfire {
    inline void bloom(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c) {
        const auto centre = b.getCentre().translated(0.0f, b.getHeight() * 0.10f);
        const float rad = juce::jmin(b.getWidth(), b.getHeight()) * 0.42f;
        juce::ColourGradient gr(c.withAlpha(0.34f), centre.x, centre.y,
                                c.withAlpha(0.0f), centre.x, centre.y + rad, true);
        gr.addColour(0.55, c.withAlpha(0.11f));
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
