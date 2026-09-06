#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <juce_graphics/juce_graphics.h>

#if JUCE_MAC
#include <CoreVideo/CoreVideo.h>
#endif

#include "hum/Capabilities.h"

namespace hum {
namespace framesample {

struct Square {
    float side = 1.0f, ox = 0.0f, oy = 0.0f;
    int w = 1, h = 1;
    float pixelX(float u) const { return u * side - ox; }
    float pixelY(float v) const { return v * side - oy; }
    float frameX(float u) const { return pixelX(u) / (float) w; }
    float frameY(float v) const { return pixelY(v) / (float) h; }
};

inline Square squareFor(const juce::Image& img) {
    Square s;
    s.w = img.getWidth();
    s.h = img.getHeight();
    s.side = (float) std::max(s.w, s.h);
    s.ox = (s.side - (float) s.w) * 0.5f;
    s.oy = (s.side - (float) s.h) * 0.5f;
    return s;
}

inline void sampleCrop(const juce::Image& src, const Square& sq, float cx, float cy,
                       float size, float angle, int side, std::vector<float>& out) {
    out.assign((size_t) side * side * 3, 0.0f);
    const juce::Image::BitmapData bd(src, juce::Image::BitmapData::readOnly);
    const float ca = std::cos(angle), sa = std::sin(angle);
    for (int y = 0; y < side; ++y)
        for (int x = 0; x < side; ++x) {
            const float u = ((float) x + 0.5f) / (float) side - 0.5f;
            const float v = ((float) y + 0.5f) / (float) side - 0.5f;
            const float rx = u * ca - v * sa, ry = u * sa + v * ca;
            const int px = (int) sq.pixelX(cx + rx * size);
            const int py = (int) sq.pixelY(cy + ry * size);
            if (px < 0 || px >= bd.width || py < 0 || py >= bd.height) continue;
            const juce::Colour c = bd.getPixelColour(px, py);
            float* p = out.data() + ((size_t) y * side + x) * 3;
            p[0] = (float) c.getRed() / 255.0f;
            p[1] = (float) c.getGreen() / 255.0f;
            p[2] = (float) c.getBlue() / 255.0f;
        }
}

struct View {
    int w = 0, h = 0, strideBytes = 0, pixelBytes = 4;
    const std::uint8_t* base = nullptr;
    int rOff = 0, bOff = 2;
};

inline View viewOf(const VideoFrameSink::Picture& src) {
    View v;
    v.w = src.width;
    v.h = src.height;
    v.base = src.pixels;
    v.strideBytes = src.width * 4;
    v.rOff = src.bgra ? 2 : 0;
    v.bOff = src.bgra ? 0 : 2;
    return v;
}

#if JUCE_MAC
class NativeLock {
public:
    explicit NativeLock(const VideoFrameSink::Picture& src) {
        if (src.native == nullptr) return;
        pb_ = (CVPixelBufferRef) src.native;
        if (CVPixelBufferLockBaseAddress(pb_, kCVPixelBufferLock_ReadOnly)
            != kCVReturnSuccess) {
            pb_ = nullptr;
            return;
        }
        view.w = (int) CVPixelBufferGetWidth(pb_);
        view.h = (int) CVPixelBufferGetHeight(pb_);
        view.strideBytes = (int) CVPixelBufferGetBytesPerRow(pb_);
        view.base = (const std::uint8_t*) CVPixelBufferGetBaseAddress(pb_);
        view.rOff = 2;
        view.bOff = 0;
    }
    ~NativeLock() {
        if (pb_ != nullptr)
            CVPixelBufferUnlockBaseAddress(pb_, kCVPixelBufferLock_ReadOnly);
    }
    View view;

private:
    CVPixelBufferRef pb_ = nullptr;
};
#else
class NativeLock {
public:
    explicit NativeLock(const VideoFrameSink::Picture&) {}
    View view;
};
#endif

inline void imageFromView(const View& s, juce::Image& into) {
    if (s.w <= 0 || s.h <= 0 || s.base == nullptr) {
        into = juce::Image();
        return;
    }
    if (into.getWidth() != s.w || into.getHeight() != s.h)
        into = juce::Image(juce::Image::ARGB, s.w, s.h, false);
    const juce::Image::BitmapData bd(into, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < s.h; ++y) {
        auto* line = (juce::PixelARGB*) bd.getLinePointer(y);
        const std::uint8_t* q = s.base + (size_t) y * (size_t) s.strideBytes;
        for (int x = 0; x < s.w; ++x, q += s.pixelBytes)
            line[x].setARGB(255, q[s.rOff], q[1], q[s.bOff]);
    }
}

inline void imageFrom(const VideoFrameSink::Picture& src, juce::Image& into) {
    if (src.native != nullptr && src.pixels == nullptr) {
        const NativeLock lock(src);
        imageFromView(lock.view, into);
        return;
    }
    imageFromView(viewOf(src), into);
}

inline juce::Image imageFromRgba(int w, int h, const std::uint8_t* rgba) {
    juce::Image img;
    imageFrom({w, h, false, rgba, nullptr, nullptr}, img);
    return img;
}

constexpr int kPreviewW = 640;

inline void previewFromView(const View& s, bool mirror, CamPreviewSource::Frame& out) {
    out = CamPreviewSource::Frame{};
    if (s.w <= 0 || s.h <= 0 || s.base == nullptr) return;
    const int pw = std::min(kPreviewW, s.w);
    const int ph = std::max(1, (int) (((long long) s.h * pw + s.w / 2) / s.w));
    const bool box = s.w >= pw * 2 && s.h >= ph * 2;
    out.width = pw;
    out.height = ph;
    out.rgba.resize((size_t) pw * (size_t) ph * 4);
    for (int y = 0; y < ph; ++y) {
        const int sy = (y * s.h) / ph;
        const auto* row0 = s.base + (size_t) sy * (size_t) s.strideBytes;
        const auto* row1 =
            s.base + (size_t) std::min(sy + 1, s.h - 1) * (size_t) s.strideBytes;
        auto* p = out.rgba.data() + (size_t) y * (size_t) pw * 4;
        for (int x = 0; x < pw; ++x, p += 4) {
            int sx = (x * s.w) / pw;
            if (mirror) sx = s.w - 1 - sx;
            const auto* a = row0 + (size_t) sx * (size_t) s.pixelBytes;
            if (box) {
                const int sx1 = mirror ? std::max(sx - 1, 0) : std::min(sx + 1, s.w - 1);
                const auto* b = row0 + (size_t) sx1 * (size_t) s.pixelBytes;
                const auto* c = row1 + (size_t) sx * (size_t) s.pixelBytes;
                const auto* d = row1 + (size_t) sx1 * (size_t) s.pixelBytes;
                p[0] = (std::uint8_t) ((a[s.rOff] + b[s.rOff] + c[s.rOff] + d[s.rOff]) >> 2);
                p[1] = (std::uint8_t) ((a[1] + b[1] + c[1] + d[1]) >> 2);
                p[2] = (std::uint8_t) ((a[s.bOff] + b[s.bOff] + c[s.bOff] + d[s.bOff]) >> 2);
            } else {
                p[0] = a[s.rOff];
                p[1] = a[1];
                p[2] = a[s.bOff];
            }
            p[3] = 255;
        }
    }
}

inline void previewFrom(const VideoFrameSink::Picture& src, bool mirror,
                        CamPreviewSource::Frame& out) {
    if (src.native != nullptr && src.pixels == nullptr) {
        const NativeLock lock(src);
        previewFromView(lock.view, mirror, out);
        return;
    }
    previewFromView(viewOf(src), mirror, out);
}

inline void previewFromImage(const juce::Image& src, bool mirror,
                             CamPreviewSource::Frame& out) {
    out = CamPreviewSource::Frame{};
    if (!src.isValid()) return;
    const auto argb = src.getFormat() == juce::Image::ARGB
                              || src.getFormat() == juce::Image::RGB
                          ? src
                          : src.convertedToFormat(juce::Image::ARGB);
    const juce::Image::BitmapData bd(argb, juce::Image::BitmapData::readOnly);
    View v;
    v.w = bd.width;
    v.h = bd.height;
    v.strideBytes = bd.lineStride;
    v.pixelBytes = bd.pixelStride;
    v.base = bd.data;
    v.rOff = 2;
    v.bOff = 0;
    previewFromView(v, mirror, out);
}

}
}
