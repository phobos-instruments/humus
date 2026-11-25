// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_graphics/juce_graphics.h>

namespace hum {

struct DragPicture {
    juce::Image image;
    juce::Rectangle<float> area;
    int widthWhenTaken = 0;
    float scaleWhenTaken = 0.0f;
    bool shown = false;

    bool taken() const { return image.isValid() && widthWhenTaken > 0; }

    void clear() { *this = {}; }

    bool wantedAt(float scaleNow) const {
        return taken() && (shown || std::abs(scaleNow - scaleWhenTaken) > 0.001f);
    }

    juce::Rectangle<float> placedAt(int widthNow) const {
        return area * ((float) widthNow / (float) widthWhenTaken);
    }

    void paint(juce::Graphics& g, int widthNow) const {
        if (!shown || !taken()) return;
        g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
        g.drawImage(image, placedAt(widthNow));
    }
};

}
