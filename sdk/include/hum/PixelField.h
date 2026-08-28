#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_graphics/juce_graphics.h>

namespace hum {

struct PixelField {
    int width = 0, height = 0;
    std::vector<float> v;

    bool empty() const { return width <= 0 || height <= 0 || v.empty(); }
    float at(int x, int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height) return 0.0f;
        return v[(std::size_t) y * (std::size_t) width + (std::size_t) x];
    }
};

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

inline void fromBytes(const juce::MemoryBlock& mb, PixelField& out, int maxW, int maxH) {
    const auto n = (int) mb.getSize();
    if (n <= 0) { out = {}; return; }
    out.width = std::clamp((int) std::sqrt((double) n), 16, maxW);
    out.height = std::clamp(n / out.width, 1, maxH);
    out.v.assign((std::size_t) out.width * (std::size_t) out.height, 0.0f);
    const auto* p = (const std::uint8_t*) mb.getData();
    const int span = out.width * out.height;
    for (int i = 0; i < span; ++i) {
        const auto at = (std::size_t) ((std::int64_t) i * n / span);
        out.v[(std::size_t) i] = (float) p[at] / 255.0f;
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
    pixelfield::fromBytes(mb, out, maxW, maxH);
    return !out.empty();
}

}
