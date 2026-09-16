// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

struct LayoutSpec;

struct SkinLayer {
    enum class Shape { Rect, Lines, Screws, Lamp, Text };
    enum class Theme { Any, Dark, Light };

    Shape shape = Shape::Rect;
    Theme theme = Theme::Any;
    float x = 0.0f, y = 0.0f, w = -1.0f, h = -1.0f;
    juce::Colour fill, fillTo;
    bool gradient = false;
    juce::Colour stroke;
    float strokeWidth = 0.0f, radius = 0.0f;
    float pitch = 3.0f, offset = 1.0f, margin = 0.0f;
    float inset = 7.0f, size = 3.0f, spread = 0.0f;
    juce::Colour mark;
    juce::String text;
    float fontSize = 12.0f;
    bool bold = false;
    juce::Justification justify = juce::Justification::centredLeft;
    juce::Colour shadow;
};

class Skin {
public:
    using Anchors = std::function<bool(const std::string& param, juce::Rectangle<float>& area)>;

    static Skin parse(const std::string& json, std::vector<std::string>* problems = nullptr,
                      const Anchors& anchors = {});
    static Skin forBlueprint(const LayoutSpec& spec, std::vector<std::string>* problems = nullptr);

    bool empty() const { return panel_.empty() && face_.empty(); }
    int layerCount() const { return (int) (panel_.size() + face_.size()); }

    void paintPanel(juce::Graphics& g, juce::Rectangle<float> whole) const { paint(g, panel_, whole); }
    void paintFace(juce::Graphics& g, juce::Rectangle<float> face) const { paint(g, face_, face); }

    static bool lightTheme();

private:
    static void paint(juce::Graphics& g, const std::vector<SkinLayer>& layers,
                      juce::Rectangle<float> area);

    std::vector<SkinLayer> panel_, face_;
};

}
