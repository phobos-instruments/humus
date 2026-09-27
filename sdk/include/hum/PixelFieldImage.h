// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

#include <juce_graphics/juce_graphics.h>

#include "hum/PixelField.h"

namespace hum {

namespace pixelfield {

inline float luma(juce::Colour c) {
    return 0.299f * c.getFloatRed() + 0.587f * c.getFloatGreen() + 0.114f * c.getFloatBlue();
}

inline void fromImage(const juce::Image& img, PixelField& out, int maxW, int maxH) {
    const int iw = img.getWidth(), ih = img.getHeight();
    out.width = std::min(maxW, std::max(1, iw));
    out.height = std::min(maxH, std::max(1, ih));
    out.v.assign((std::size_t) out.width * (std::size_t) out.height, 0.0f);
    const juce::Image::BitmapData bd(img, juce::Image::BitmapData::readOnly);
    for (int y = 0; y < out.height; ++y) {
        const int y0 = y * ih / out.height, y1 = std::max(y0 + 1, (y + 1) * ih / out.height);
        for (int x = 0; x < out.width; ++x) {
            const int x0 = x * iw / out.width, x1 = std::max(x0 + 1, (x + 1) * iw / out.width);
            float sum = 0.0f;
            int n = 0;
            for (int yy = y0; yy < y1; ++yy)
                for (int xx = x0; xx < x1; ++xx) { sum += luma(bd.getPixelColour(xx, yy)); ++n; }
            out.v[(std::size_t) y * (std::size_t) out.width + (std::size_t) x] =
                n > 0 ? sum / (float) n : 0.0f;
        }
    }
}

}

inline bool loadPixelField(const std::string& path, PixelField& out,
                           int maxW = 1024, int maxH = 512) {
    out = {};
    const juce::File f(juce::String::fromUTF8(path.c_str()));
    if (path.empty() || !f.existsAsFile()) return false;
    if (auto img = juce::ImageFileFormat::loadFrom(f); img.isValid()) {
        pixelfield::fromImage(img, out, maxW, maxH);
        return true;
    }
    juce::MemoryBlock mb;
    if (!f.loadFileAsData(mb)) return false;
    pixelfield::fromBytes(static_cast<const std::uint8_t*>(mb.getData()), mb.getSize(), out, maxW, maxH);
    return !out.empty();
}

}
