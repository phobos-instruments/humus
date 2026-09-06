#pragma once
#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

inline const juce::Image& humusLogo() {
    static const juce::Image img = juce::ImageCache::getFromMemory(
        BinaryData::logo_png, BinaryData::logo_pngSize);
    return img;
}

inline const juce::Image& humusMark() {
    static const juce::Image img = juce::ImageCache::getFromMemory(
        BinaryData::icon1024_png, BinaryData::icon1024_pngSize);
    return img;
}

inline constexpr float kBrandRadius = 12.0f;

inline void paintBrandPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                            juce::Colour fill, juce::Colour edge) {
    juce::Graphics::ScopedSaveState keep(g);
    const auto r = bounds.toFloat().reduced(0.5f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, kBrandRadius);
    g.setColour(edge.withAlpha(0.18f));
    g.drawRoundedRectangle(r, kBrandRadius, 1.0f);
}

inline void drawHumusLogo(juce::Graphics& g, juce::Rectangle<float> bounds) {
    juce::Graphics::ScopedSaveState keep(g);
    g.setColour(juce::Colours::white);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(humusLogo(), bounds, juce::RectanglePlacement::centred);
}

inline void drawHumusMark(juce::Graphics& g, juce::Rectangle<float> bounds) {
    juce::Graphics::ScopedSaveState keep(g);
    g.setColour(juce::Colours::white);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(humusMark(), bounds, juce::RectanglePlacement::centred);
}

}
