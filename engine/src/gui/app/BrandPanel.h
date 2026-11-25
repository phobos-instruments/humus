// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <map>

#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"

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

inline juce::Rectangle<int> logoEmblemArea() { return {25, 18, 622, 622}; }

inline const juce::Image& humusEmblemAt(int pixels, juce::Colour colour) {
    static std::map<juce::int64, juce::Image> cache;
    const juce::int64 key = ((juce::int64) pixels << 32) ^ (juce::int64) (juce::uint32) colour.getARGB();
    if (auto it = cache.find(key); it != cache.end()) return it->second;
    if (cache.size() >= 32) cache.clear();
    juce::Image img(juce::Image::ARGB, pixels, pixels, true, juce::SoftwareImageType());
    juce::Graphics ig(img);
    const auto src = logoEmblemArea();
    ig.setColour(colour);
    ig.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    ig.drawImage(humusLogo(), 0, 0, pixels, pixels, src.getX(), src.getY(), src.getWidth(), src.getHeight(), true);
    return cache.emplace(key, std::move(img)).first->second;
}

inline void drawHumusEmblem(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour) {
    const float side = std::min(bounds.getWidth(), bounds.getHeight());
    const auto dst = bounds.withSizeKeepingCentre(side, side).toNearestInt();
    if (dst.isEmpty() || !g.clipRegionIntersects(dst)) return;
    const float scale = juce::jmax(0.1f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int pixels = juce::jmax(1, juce::roundToInt((float) dst.getWidth() * scale));
    juce::Graphics::ScopedSaveState keep(g);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImageTransformed(humusEmblemAt(pixels, colour),
                           juce::AffineTransform::scale((float) dst.getWidth() / (float) pixels)
                               .translated((float) dst.getX(), (float) dst.getY()));
}

inline constexpr float kBrandRadius = 12.0f;

inline void paintBrandPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                            juce::Colour fill, juce::Colour edge) {
    juce::Graphics::ScopedSaveState keep(g);
    const auto r = bounds.toFloat().reduced(0.5f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, kBrandRadius);
    g.setColour(edge.withAlpha(alpha::veil));
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
