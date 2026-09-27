// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/ControlArt.h"
#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"

namespace hum {

struct ArtImage {
    juce::Image image;
    float scale = 1.0f;

    bool valid() const { return image.isValid(); }
    float width() const { return (float) image.getWidth() / scale; }
    float height() const { return (float) image.getHeight() / scale; }

    static ArtImage load(const std::string& path) {
        ArtImage a;
        if (path.empty()) return a;
        const juce::File file(juce::String::fromUTF8(path.c_str()));
        const auto retina = file.getSiblingFile(file.getFileNameWithoutExtension() + "@2x" + file.getFileExtension());
        if (retina.existsAsFile()) {
            a.image = juce::ImageCache::getFromFile(retina);
            a.scale = 2.0f;
        }
        if (!a.image.isValid()) {
            a.image = juce::ImageCache::getFromFile(file);
            a.scale = 1.0f;
        }
        return a;
    }
};

inline void drawArtRegion(juce::Graphics& g, const ArtImage& art, juce::Rectangle<float> dest,
                          juce::Rectangle<float> source) {
    const auto s = source * art.scale;
    g.drawImage(art.image, juce::roundToInt(dest.getX()), juce::roundToInt(dest.getY()),
                juce::roundToInt(dest.getWidth()), juce::roundToInt(dest.getHeight()), juce::roundToInt(s.getX()),
                juce::roundToInt(s.getY()), juce::roundToInt(s.getWidth()), juce::roundToInt(s.getHeight()));
}

inline void drawSliced(juce::Graphics& g, const ArtImage& art, juce::Rectangle<float> dest, float slice) {
    if (slice <= 0.0f) {
        g.drawImage(art.image, dest, juce::RectanglePlacement::stretchToFit);
        return;
    }
    const float w = art.width(), h = art.height();
    const float sx = std::min(slice, std::min(w, dest.getWidth()) * 0.5f);
    const float sy = std::min(slice, std::min(h, dest.getHeight()) * 0.5f);
    const float srcX[] = {0.0f, sx, w - sx, w};
    const float srcY[] = {0.0f, sy, h - sy, h};
    const float dstX[] = {dest.getX(), dest.getX() + sx, dest.getRight() - sx, dest.getRight()};
    const float dstY[] = {dest.getY(), dest.getY() + sy, dest.getBottom() - sy, dest.getBottom()};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col) {
            const juce::Rectangle<float> src(srcX[col], srcY[row], srcX[col + 1] - srcX[col], srcY[row + 1] - srcY[row]);
            const juce::Rectangle<float> dst(dstX[col], dstY[row], dstX[col + 1] - dstX[col], dstY[row + 1] - dstY[row]);
            if (src.getWidth() > 0.0f && src.getHeight() > 0.0f && dst.getWidth() > 0.0f && dst.getHeight() > 0.0f)
                drawArtRegion(g, art, dst, src);
        }
}

inline juce::Colour artTextColour(const ControlArt& art, juce::Colour fallback) {
    auto t = juce::String(art.text).trim().trimCharactersAtStart("#");
    if ((t.length() != 6 && t.length() != 8) || !t.containsOnly("0123456789abcdefABCDEF")) return fallback;
    auto argb = (juce::uint32) t.getHexValue32();
    if (t.length() == 6) argb |= 255u << 24;
    return juce::Colour(argb);
}

class ButtonArt {
public:
    explicit ButtonArt(const ControlArt& art)
        : off_(ArtImage::load(art.image)), on_(ArtImage::load(art.on)), down_(ArtImage::load(art.down)),
          slice_((float) art.slice), caption_(art.caption),
          text_(artTextColour(art, ink::brand::ground)) {}

    bool valid() const { return off_.valid(); }
    bool caption() const { return caption_; }
    juce::Colour text() const { return text_; }

    void paintPlate(juce::Graphics& g, juce::Button& b, bool down) const {
        const auto& art = down && down_.valid() ? down_ : b.getToggleState() && on_.valid() ? on_ : off_;
        drawSliced(g, art, b.getLocalBounds().toFloat(), slice_);
    }

    void paintCaption(juce::Graphics& g, juce::Button& b, const std::optional<IconGlyph>& icon) const {
        if (!caption_) return;
        const auto colour = text_.withMultipliedAlpha(b.isEnabled() ? 1.0f : 0.5f);
        g.setColour(colour);
        if (icon) {
            const float side = std::min((float) b.getWidth(), (float) b.getHeight()) * 0.5f;
            drawIconGlyph(g, *icon, b.getLocalBounds().toFloat().withSizeKeepingCentre(side, side), colour, b.isEnabled(),
                          b.getToggleState());
            return;
        }
        g.setFont(juce::FontOptions(std::min(15.0f, b.getHeight() * 0.5f), juce::Font::bold));
        g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(3, 1), juce::Justification::centred, 1);
    }

private:
    ArtImage off_, on_, down_;
    float slice_ = 0.0f;
    bool caption_ = true;
    juce::Colour text_;
};

class ComboArt {
public:
    explicit ComboArt(const ControlArt& art)
        : plate_(ArtImage::load(art.image)), open_(ArtImage::load(art.down)), arrow_(ArtImage::load(art.arrow)),
          slice_((float) art.slice), text_(artTextColour(art, ink::brand::ground)) {}

    bool valid() const { return plate_.valid(); }
    juce::Colour text() const { return text_; }

    void paint(juce::Graphics& g, juce::ComboBox& box) const {
        const auto bounds = box.getLocalBounds().toFloat();
        drawSliced(g, box.isPopupActive() && open_.valid() ? open_ : plate_, bounds, slice_);
        if (!arrow_.valid()) return;
        const auto well = bounds.withLeft(bounds.getRight() - bounds.getHeight() * 0.9f);
        const float scale = std::min({1.0f, well.getWidth() / arrow_.width(), well.getHeight() / arrow_.height()});
        g.drawImage(arrow_.image, well.withSizeKeepingCentre(arrow_.width() * scale, arrow_.height() * scale),
                    juce::RectanglePlacement::stretchToFit);
    }

private:
    ArtImage plate_, open_, arrow_;
    float slice_ = 0.0f;
    juce::Colour text_;
};

class KnobArt {
public:
    explicit KnobArt(const ControlArt& art)
        : image_(ArtImage::load(art.image)), track_(ArtImage::load(art.track)), frames_(art.frames),
          from_((float) juce::degreesToRadians(art.fromAngle)), to_((float) juce::degreesToRadians(art.toAngle)) {}

    bool valid() const { return image_.valid(); }

    void paintRotary(juce::Graphics& g, juce::Rectangle<float> area, float proportion) const {
        const float side = std::min(area.getWidth(), area.getHeight());
        const auto dest = area.withSizeKeepingCentre(side, side);
        if (frames_ > 1) {
            const bool vertical = image_.height() >= image_.width();
            const float frameW = vertical ? image_.width() : image_.width() / (float) frames_;
            const float frameH = vertical ? image_.height() / (float) frames_ : image_.height();
            const int frame = juce::jlimit(0, frames_ - 1, juce::roundToInt(proportion * (float) (frames_ - 1)));
            const juce::Rectangle<float> src(vertical ? 0.0f : frameW * (float) frame,
                                             vertical ? frameH * (float) frame : 0.0f, frameW, frameH);
            drawArtRegion(g, image_, dest, src);
            return;
        }
        const float angle = from_ + proportion * (to_ - from_);
        const auto placed = juce::RectanglePlacement(juce::RectanglePlacement::centred)
                                .getTransformToFit(image_.image.getBounds().toFloat(), dest);
        g.drawImageTransformed(image_.image, placed.rotated(angle, dest.getCentreX(), dest.getCentreY()));
    }

    void paintLinear(juce::Graphics& g, juce::Rectangle<float> area, float position, bool vertical) const {
        if (track_.valid()) g.drawImage(track_.image, area, juce::RectanglePlacement::stretchToFit);
        const float capW = std::min(image_.width(), area.getWidth());
        const float capH = std::min(image_.height(), area.getHeight());
        const juce::Rectangle<float> cap(capW, capH);
        const auto centre = vertical ? juce::Point<float>(area.getCentreX(), position)
                                     : juce::Point<float>(position, area.getCentreY());
        g.drawImage(image_.image, cap.withCentre(centre), juce::RectanglePlacement::stretchToFit);
    }

private:
    ArtImage image_, track_;
    int frames_ = 0;
    float from_ = 0.0f, to_ = 0.0f;
};

}
