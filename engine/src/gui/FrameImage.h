#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/DxtDecode.h"
#include "gui/VideoLayer.h"

namespace hum {

inline bool frameToBgra(const VideoLayer::Frame& f, std::vector<std::uint8_t>& decoded,
                        const std::uint8_t*& bgra) {
    const std::size_t need = (std::size_t) f.width * (std::size_t) f.height * 4;
    if (f.width <= 0 || f.height <= 0) return false;
    if (f.fmt == VideoLayer::Frame::BGRA) {
        if (f.bgra.size() >= need) {
            bgra = f.bgra.data();
            return true;
        }
        if (!f.readPixels || !f.readPixels(decoded) || decoded.size() < need) return false;
        bgra = decoded.data();
        return true;
    }
    const auto kind = f.fmt == VideoLayer::Frame::DXT1 ? dxt::Kind::DXT1
                      : f.fmt == VideoLayer::Frame::DXT5 ? dxt::Kind::DXT5
                                                         : dxt::Kind::YCoCgDXT5;
    if (!dxt::decodeToBgra(kind, f.blocks.data(), f.blocks.size(), f.width, f.height, decoded))
        return false;
    bgra = decoded.data();
    return true;
}

inline juce::Image imageOfFrame(const VideoLayer::Frame& f, int width, int height) {
    if (width <= 0 || height <= 0) return {};
    std::vector<std::uint8_t> decoded;
    const std::uint8_t* bgra = nullptr;
    if (!frameToBgra(f, decoded, bgra)) return {};
    juce::Image img(juce::Image::RGB, width, height, false);
    juce::Image::BitmapData bd(img, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < bd.height; ++y) {
        const int sy = y * f.height / bd.height;
        for (int x = 0; x < bd.width; ++x) {
            const int sx = x * f.width / bd.width;
            const auto* px = bgra + ((std::size_t) sy * (std::size_t) f.width + (std::size_t) sx) * 4;
            bd.setPixelColour(x, y, juce::Colour(px[2], px[1], px[0]));
        }
    }
    return img;
}

}
