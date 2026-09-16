// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
// macOS closed-hand cursor (see HandCursors.h): renders NSCursor.closedHandCursor's
// image into a juce::Image once and serves it as a cached JUCE cursor, so the
// grab-pressed state matches the system's own grabbing hand exactly.
#include "gui/style/HandCursors.h"

#include <cmath>

#if JUCE_MAC

#import <AppKit/AppKit.h>

namespace hum {

juce::MouseCursor grabbingHandCursor() {
    static const juce::MouseCursor cursor = [] {
        NSCursor* hand = [NSCursor closedHandCursor];
        NSImage* img = [hand image];
        const int w = (int) std::ceil([img size].width);
        const int h = (int) std::ceil([img size].height);
        if (w <= 0 || h <= 0)
            return juce::MouseCursor(juce::MouseCursor::DraggingHandCursor);

        // Render at 2x so Retina displays get the crisp representation.
        constexpr int scale = 2;
        juce::Image out(juce::Image::ARGB, w * scale, h * scale, true);
        {
            juce::Image::BitmapData bd(out, juce::Image::BitmapData::writeOnly);
            CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
            CGContextRef ctx = CGBitmapContextCreate(
                bd.data, (size_t) (w * scale), (size_t) (h * scale), 8,
                (size_t) bd.lineStride, cs,
                kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
            CGColorSpaceRelease(cs);
            if (ctx == nullptr)
                return juce::MouseCursor(juce::MouseCursor::DraggingHandCursor);
            NSGraphicsContext* g =
                [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:NO];
            [NSGraphicsContext saveGraphicsState];
            [NSGraphicsContext setCurrentContext:g];
            [img drawInRect:NSMakeRect(0, 0, w * scale, h * scale)
                   fromRect:NSZeroRect
                  operation:NSCompositingOperationSourceOver
                   fraction:1.0];
            [NSGraphicsContext restoreGraphicsState];
            CGContextRelease(ctx);
        }
        const NSPoint hot = [hand hotSpot];   // top-left origin, in points
        return juce::MouseCursor(juce::ScaledImage(out, (double) scale),
                                 {(int) hot.x, (int) hot.y});
    }();
    return cursor;
}

}  // namespace hum

#endif  // JUCE_MAC
