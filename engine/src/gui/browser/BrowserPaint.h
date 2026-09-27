// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserEntry.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/Contrast.h"
#include "gui/style/LookAndFeel.h"

namespace hum::browser::paint {

inline constexpr float kStarPitch = 13.0f, kStarSize = 10.0f;

inline juce::Colour kindColour(Kind kind) {
    switch (kind) {
        case Kind::Sound: case Kind::Video: return Palette::familyAccent(Family::Motion);
        case Kind::Impulse: case Kind::Bank: case Kind::Preset: return Palette::familyAccent(Family::Sense);
        case Kind::Midi: case Kind::Scale: return Palette::familyAccent(Family::Time);
        case Kind::Project: case Kind::Patch: case Kind::Shader: return Palette::familyAccent(Family::Voice);
        case Kind::Folder: case Kind::Other: break;
    }
    return Palette::textDim;
}

inline float badgeWidth(const juce::String& word) {
    return juce::GlyphArrangement::getStringWidth(juce::FontOptions(9.5f, juce::Font::bold), word) + 16.0f;
}

inline float drawBadge(juce::Graphics& g, Kind kind, float x, float centreY) {
    const juce::String word(kindBadge(kind));
    const float w = badgeWidth(word);
    const juce::Rectangle<float> r(x, centreY - 7.5f, w, 15.0f);
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(kindColour(kind));
    g.fillEllipse(r.getX() + 5.0f, centreY - 2.0f, 4.0f, 4.0f);
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText(word, r.withTrimmedLeft(11.0f).withTrimmedRight(3.0f), juce::Justification::centredLeft, false);
    return w;
}

inline void drawStars(juce::Graphics& g, int rating, float x, float centreY, bool hot) {
    for (int i = 0; i < kMaxRating; ++i) {
        juce::Path star;
        const float cx = x + kStarPitch * (float) i + kStarSize * 0.5f;
        star.addStar({cx, centreY}, 5, kStarSize * 0.22f, kStarSize * 0.5f);
        const bool lit = i < rating;
        g.setColour(lit ? Palette::text : Palette::textDim.withAlpha(hot ? alpha::mid : alpha::muted));
        if (lit) g.fillPath(star);
        else g.strokePath(star, juce::PathStrokeType(0.8f));
    }
}

inline int starsAt(float x, float left) {
    const int i = (int) std::floor((x - left) / kStarPitch);
    return i < 0 ? 0 : std::min(kMaxRating, i + 1);
}

inline void drawHeart(juce::Graphics& g, juce::Rectangle<float> r, bool on, bool hot) {
    if (!on && !hot) return;
    const float s = std::min(r.getWidth(), r.getHeight()) * 0.55f;
    const auto c = r.getCentre();
    juce::Path heart;
    heart.startNewSubPath(c.x, c.y + s * 0.45f);
    heart.cubicTo(c.x - s * 0.9f, c.y - s * 0.1f, c.x - s * 0.45f, c.y - s * 0.75f, c.x, c.y - s * 0.3f);
    heart.cubicTo(c.x + s * 0.45f, c.y - s * 0.75f, c.x + s * 0.9f, c.y - s * 0.1f, c.x, c.y + s * 0.45f);
    heart.closeSubPath();
    g.setColour(on ? Palette::accent : Palette::textDim.withAlpha(alpha::mid));
    if (on) g.fillPath(heart);
    else g.strokePath(heart, juce::PathStrokeType(1.0f));
}

inline void drawPeaks(juce::Graphics& g, const std::vector<std::uint8_t>& peaks, juce::Rectangle<float> r,
                      juce::Colour colour) {
    if (peaks.empty()) return;
    const float step = r.getWidth() / (float) peaks.size();
    g.setColour(colour);
    for (size_t i = 0; i < peaks.size(); ++i) {
        const float h = std::max(1.0f, r.getHeight() * (float) peaks[i] / 255.0f);
        g.fillRect(r.getX() + step * (float) i, r.getCentreY() - h * 0.5f, std::max(1.0f, step - 1.0f), h);
    }
}

inline juce::Colour onAccent() {
    return contrast::ratio(Palette::accent, Palette::text) >= contrast::ratio(Palette::accent, Palette::background)
               ? Palette::text : Palette::background;
}

inline juce::String systemDialogLabel() { return tr("browser.use-file-browser", "Use File Browser..."); }

inline juce::String revealLabel() {
#if JUCE_MAC
    return tr("browser.reveal", "Show in Finder");
#elif JUCE_WINDOWS
    return tr("browser.reveal-explorer", "Show in Explorer");
#else
    return tr("browser.reveal-file", "Show the file");
#endif
}

inline juce::String lengthText(double seconds) {
    if (seconds <= 0.0) return {};
    if (seconds < 10.0) return juce::String(seconds, 2) + " s";
    const int whole = (int) std::lround(seconds);
    return juce::String(whole / 60) + ":" + juce::String(whole % 60).paddedLeft('0', 2);
}

inline juce::String sizeText(std::int64_t bytes) {
    if (bytes <= 0) return {};
    if (bytes < 1024 * 1024) return juce::String(std::max<std::int64_t>(1, bytes / 1024)) + " KB";
    return juce::String((double) bytes / (1024.0 * 1024.0), 1) + " MB";
}

inline juce::String rateText(double rate) {
    if (rate <= 0.0) return {};
    const double k = rate / 1000.0;
    return (std::abs(k - std::round(k)) < 0.05 ? juce::String((int) std::lround(k)) : juce::String(k, 1)) + " kHz";
}

inline juce::String dateText(std::int64_t seconds) {
    if (seconds <= 0) return {};
    return juce::Time(seconds * 1000).formatted("%d %b %Y");
}

}
