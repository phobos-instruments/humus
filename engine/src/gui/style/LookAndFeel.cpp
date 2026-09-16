// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

namespace {
struct ThemePreset {
    const char* name;
    juce::uint32 background, panel, panelLight, border, accent, accentDim, text, textDim, cord;
};
const ThemePreset kThemes[] = {
    {"Humus", 0xff171410, 0xff231e16, 0xff2f2920, 0xff473e2e,
     0xff9fbf5a, 0xff5c6e34, 0xffefe6d2, 0xffa39a83, 0xff7a8a4e},
    {"Humus Light", 0xfff0e6d2, 0xffe6dbc2, 0xffdacfb2, 0xffb5a68a,
     0xff6b7d33, 0xffa9b585, 0xff33402a, 0xff6b6a52, 0xff7e8f3c},
    {"Terracotta", 0xff1a1210, 0xff281c17, 0xff362620, 0xff523a2f,
     0xffe07a4a, 0xff7d4326, 0xffefdfd2, 0xffa78f7f, 0xff96604a},
    {"One Dark", 0xff21252b, 0xff282c34, 0xff2f343e, 0xff3e4451,
     0xff61afef, 0xff3a6a94, 0xffabb2bf, 0xff7a828e, 0xff98c379},
    {"Rose Pine", 0xff191724, 0xff1f1d2e, 0xff26233a, 0xff403d52,
     0xffebbcba, 0xff8a6c6b, 0xffe0def4, 0xff908caa, 0xff9ccfd8},
};
constexpr int kThemeCount = (int) (sizeof(kThemes) / sizeof(kThemes[0]));
}

int numThemes() { return kThemeCount; }
const char* themeName(int i) { return (i >= 0 && i < kThemeCount) ? kThemes[i].name : ""; }

ThemeColours presetColours(int i) {
    const auto& t = kThemes[(i >= 0 && i < kThemeCount) ? i : 0];
    return {juce::Colour(t.background), juce::Colour(t.panel), juce::Colour(t.panelLight),
            juce::Colour(t.border), juce::Colour(t.accent), juce::Colour(t.accentDim),
            juce::Colour(t.text), juce::Colour(t.textDim), juce::Colour(t.cord)};
}

namespace {
HumLookAndFeel* gLaf = nullptr;
ThemeColours gBase = presetColours(0);
double gBrightness = 0.0;
double gContrast = 0.0;

juce::Colour adjust(juce::Colour c) {
    const double k = gContrast + 1.0;
    auto f = [&](float v) {
        double x = (v - 0.5) * k + 0.5 + gBrightness;
        return (float) juce::jlimit(0.0, 1.0, x);
    };
    return juce::Colour::fromFloatRGBA(f(c.getFloatRed()), f(c.getFloatGreen()),
                                       f(c.getFloatBlue()), c.getFloatAlpha());
}
}

void recomputePalette() {
    Palette::background = adjust(gBase.background);
    Palette::panel      = adjust(gBase.panel);
    Palette::panelLight = adjust(gBase.panelLight);
    Palette::border     = adjust(gBase.border);
    Palette::accent     = adjust(gBase.accent);
    Palette::accentDim  = adjust(gBase.accentDim);
    Palette::text       = adjust(gBase.text);
    Palette::textDim    = adjust(gBase.textDim);
    Palette::cord       = adjust(gBase.cord);
    Palette::derivedMidiCord = Palette::cord.withRotatedHue(-0.22f).brighter(0.15f);
    Palette::derivedVideoCord = Palette::cord.withRotatedHue(0.35f).brighter(0.25f);
    const juce::Colour family[5] = {
        Palette::accent,
        Palette::accent.withRotatedHue(0.225f).brighter(0.12f),
        Palette::accent.withRotatedHue(-0.105f).brighter(0.04f),
        Palette::accent.withRotatedHue(0.525f).brighter(0.20f),
        Palette::textDim,
    };
    for (int i = 0; i < 5; ++i) {
        Palette::derivedFamily[i] = family[i];
        Palette::derivedFamilyDim[i] = i == (int) Family::Utility
                                           ? Palette::textDim.darker(0.45f)
                                           : family[i].darker(0.72f);
    }
    if (gLaf != nullptr) gLaf->refreshColours();
}

void applyTheme(int i) {
    if (i < 0 || i >= kThemeCount) return;
    gBase = presetColours(i);
    recomputePalette();
}

void setBaseColours(const ThemeColours& c) { gBase = c; recomputePalette(); }
ThemeColours baseColours() { return gBase; }

void setAppearanceAdjustments(double brightness, double contrast) {
    gBrightness = juce::jlimit(-1.0, 1.0, brightness);
    gContrast   = juce::jlimit(-1.0, 1.0, contrast);
    recomputePalette();
}
double brightnessAdjustment() { return gBrightness; }
double contrastAdjustment() { return gContrast; }

HumLookAndFeel::HumLookAndFeel() { gLaf = this; recomputePalette(); }
HumLookAndFeel::~HumLookAndFeel() { if (gLaf == this) gLaf = nullptr; }

void HumLookAndFeel::refreshColours() {
    setColour(juce::ResizableWindow::backgroundColourId, Palette::background);
    setColour(juce::DocumentWindow::textColourId, Palette::text);

    setColour(juce::Label::textColourId, Palette::text);

    setColour(juce::Slider::textBoxTextColourId, Palette::text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::rotarySliderFillColourId, Palette::accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, Palette::panelLight);
    setColour(juce::Slider::thumbColourId, Palette::accent);
    setColour(juce::Slider::trackColourId, Palette::accent);
    setColour(juce::Slider::backgroundColourId, Palette::panelLight);

    setColour(juce::TextButton::buttonColourId, Palette::panelLight);
    setColour(juce::TextButton::buttonOnColourId, Palette::accentDim);
    setColour(juce::TextButton::textColourOffId, Palette::text);
    setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    setColour(juce::ToggleButton::textColourId, Palette::text);
    setColour(juce::ToggleButton::tickColourId, Palette::accent);
    setColour(juce::ToggleButton::tickDisabledColourId, Palette::border);

    setColour(juce::ComboBox::backgroundColourId, Palette::panelLight);
    setColour(juce::ComboBox::textColourId, Palette::text);

    setColour(juce::PopupMenu::backgroundColourId, Palette::panel.withAlpha(alpha::nearOpaque));
    setColour(juce::PopupMenu::textColourId, Palette::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Palette::accentDim);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour(juce::ListBox::backgroundColourId, Palette::panel);
    setColour(juce::ScrollBar::thumbColourId, Palette::border);

    setColour(juce::BubbleComponent::backgroundColourId, Palette::panelLight);
    setColour(juce::BubbleComponent::outlineColourId, Palette::border);
    setColour(juce::TooltipWindow::backgroundColourId, Palette::panelLight);
    setColour(juce::TooltipWindow::textColourId, Palette::text);
    setColour(juce::TooltipWindow::outlineColourId, Palette::border);

    setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
    setColour(juce::TextEditor::textColourId, Palette::text);
    setColour(juce::TextEditor::outlineColourId, Palette::border);
    setColour(juce::TextEditor::focusedOutlineColourId, Palette::accent);
    setColour(juce::TextEditor::highlightColourId, Palette::accentDim);
    setColour(juce::TextEditor::highlightedTextColourId, Palette::text);
    setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    setColour(juce::CaretComponent::caretColourId, Palette::accent);

    setColour(juce::AlertWindow::backgroundColourId, Palette::panel);
    setColour(juce::AlertWindow::textColourId, Palette::text);
    setColour(juce::AlertWindow::outlineColourId, Palette::border);
    setColour(juce::ProgressBar::backgroundColourId, Palette::panelLight);
    setColour(juce::ProgressBar::foregroundColourId, Palette::accent);
    setColour(juce::TableHeaderComponent::backgroundColourId, Palette::panelLight);
    setColour(juce::TableHeaderComponent::textColourId, Palette::text);
    setColour(juce::TableHeaderComponent::outlineColourId, Palette::border);
}

}
