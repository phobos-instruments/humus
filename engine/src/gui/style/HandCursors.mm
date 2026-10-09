// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/HandCursors.h"

#include <cmath>

#if JUCE_MAC

#import <AppKit/AppKit.h>

namespace hum {

namespace {

juce::MouseCursor systemCursor(NSCursor* system, juce::MouseCursor::StandardCursorType fallback) {
    NSImage* img = [system image];
    const int w = (int) std::ceil([img size].width);
    const int h = (int) std::ceil([img size].height);
    if (w <= 0 || h <= 0) return juce::MouseCursor(fallback);

    constexpr int scale = 2;
    juce::Image out(juce::Image::ARGB, w * scale, h * scale, true);
    {
        juce::Image::BitmapData bd(out, juce::Image::BitmapData::writeOnly);
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(bd.data, (size_t) (w * scale), (size_t) (h * scale), 8,
                                                 (size_t) bd.lineStride, cs,
                                                 kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
        CGColorSpaceRelease(cs);
        if (ctx == nullptr) return juce::MouseCursor(fallback);
        NSGraphicsContext* g = [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:NO];
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:g];
        [img drawInRect:NSMakeRect(0, 0, w * scale, h * scale)
               fromRect:NSZeroRect
              operation:NSCompositingOperationSourceOver
               fraction:1.0];
        [NSGraphicsContext restoreGraphicsState];
        CGContextRelease(ctx);
    }
    const NSPoint hot = [system hotSpot];
    return juce::MouseCursor(juce::ScaledImage(out, (double) scale), {(int) hot.x, (int) hot.y});
}

}

juce::MouseCursor grabbingHandCursor() {
    static const juce::MouseCursor cursor =
        systemCursor([NSCursor closedHandCursor], juce::MouseCursor::DraggingHandCursor);
    return cursor;
}

juce::MouseCursor notAllowedCursor() {
    static const juce::MouseCursor cursor =
        systemCursor([NSCursor operationNotAllowedCursor], juce::MouseCursor::NormalCursor);
    return cursor;
}

}

#endif
