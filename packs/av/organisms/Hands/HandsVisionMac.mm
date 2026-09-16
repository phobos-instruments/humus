// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
// Hands - macOS backends: a minimal AVCaptureSession video tap (no photo
// output: JUCE's CameraDevice attaches one whose KVO wrapper logs
// "NSKVONotifying_AVCapturePhotoOutput not linked" at every open), and
// Apple Vision's VNDetectHumanHandPoseRequest running straight off the
// capture CVPixelBuffer - no image conversion in the hot path. Vision's
// joint set maps 1:1 onto MediaPipe Hands' 21-point layout (HandPose.h),
// so the portable finger math never knows which detector ran. Everything
// runs on the capture queue; frames never leave the process.
// Apple frameworks FIRST: AVFoundation drags in ApplicationServices/QuickDraw,
// whose legacy global `Pattern` type collides if our headers (hum::Pattern via
// Organism.h) have already been parsed into the translation unit.
#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_MAC
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Vision/Vision.h>
#endif

#include "Hands/Hands.h"

#if JUCE_MAC

#include <cstdint>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>

namespace hum {

namespace {


}  // namespace

void handsPreviewFromPixelBuffer(void* pbRaw, bool mirror, CamPreviewSource::Frame& out) {
    out = CamPreviewSource::Frame{};
    auto* pb = (CVPixelBufferRef) pbRaw;
    if (pb == nullptr) return;
    if (CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess)
        return;
    const int sw = (int) CVPixelBufferGetWidth(pb);
    const int sh = (int) CVPixelBufferGetHeight(pb);
    const int stride = (int) CVPixelBufferGetBytesPerRow(pb);
    const auto* base = (const std::uint8_t*) CVPixelBufferGetBaseAddress(pb);
    if (base != nullptr && sw > 0 && sh > 0) {
        // 640 wide at the SOURCE aspect (720p capture -> 640x360; the editor
        // letterboxes), with a 2x2 box filter when the source is at least
        // twice the preview - cheap, and it kills the nearest-neighbour
        // shimmer that read as "pixelated".
        const int pw = 640;
        const int ph = std::max(1, (int) (((long) sh * pw + sw / 2) / sw));
        const bool box = sw >= pw * 2 && sh >= ph * 2;
        out.width = pw;
        out.height = ph;
        out.rgba.resize((size_t) pw * ph * 4);
        for (int y = 0; y < ph; ++y) {
            const int sy = (y * sh) / ph;
            const auto* row0 = base + (size_t) sy * (size_t) stride;
            const auto* row1 = base + (size_t) std::min(sy + 1, sh - 1) * (size_t) stride;
            for (int x = 0; x < pw; ++x) {
                int sx = (x * sw) / pw;
                if (mirror) sx = sw - 1 - sx;
                auto* p = out.rgba.data() + ((size_t) y * pw + (size_t) x) * 4;
                const auto* a = row0 + (size_t) sx * 4;   // BGRA
                if (box) {
                    const int sx1 = std::min(sx + 1, sw - 1);
                    const auto* b = row0 + (size_t) sx1 * 4;
                    const auto* c = row1 + (size_t) sx * 4;
                    const auto* d = row1 + (size_t) sx1 * 4;
                    p[0] = (std::uint8_t) ((a[2] + b[2] + c[2] + d[2]) >> 2);
                    p[1] = (std::uint8_t) ((a[1] + b[1] + c[1] + d[1]) >> 2);
                    p[2] = (std::uint8_t) ((a[0] + b[0] + c[0] + d[0]) >> 2);
                } else {
                    p[0] = a[2]; p[1] = a[1]; p[2] = a[0];
                }
                p[3] = 255;
            }
        }
    }
    CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
}

// --- Vision hand pose ----------------------------------------------------------

// Fill one observation into MediaPipe layout; true when it clears the gates.
static bool fillHand(VNHumanHandPoseObservation* obs, bool mirror, float minConfidence,
                     HandLandmarks& out) API_AVAILABLE(macos(11.0)) {
    out = HandLandmarks{};
    NSError* err = nil;
    NSDictionary<VNHumanHandPoseObservationJointName, VNRecognizedPoint*>* joints =
        [obs recognizedPointsForJointsGroupName:VNHumanHandPoseObservationJointsGroupNameAll
                                          error:&err];
    if (joints == nil) return false;

    // Vision joint -> MediaPipe index (HandPose.h layout).
    static const std::pair<VNHumanHandPoseObservationJointName, int> kMap[] = {
        {VNHumanHandPoseObservationJointNameWrist, 0},
        {VNHumanHandPoseObservationJointNameThumbCMC, 1},
        {VNHumanHandPoseObservationJointNameThumbMP, 2},
        {VNHumanHandPoseObservationJointNameThumbIP, 3},
        {VNHumanHandPoseObservationJointNameThumbTip, 4},
        {VNHumanHandPoseObservationJointNameIndexMCP, 5},
        {VNHumanHandPoseObservationJointNameIndexPIP, 6},
        {VNHumanHandPoseObservationJointNameIndexDIP, 7},
        {VNHumanHandPoseObservationJointNameIndexTip, 8},
        {VNHumanHandPoseObservationJointNameMiddleMCP, 9},
        {VNHumanHandPoseObservationJointNameMiddlePIP, 10},
        {VNHumanHandPoseObservationJointNameMiddleDIP, 11},
        {VNHumanHandPoseObservationJointNameMiddleTip, 12},
        {VNHumanHandPoseObservationJointNameRingMCP, 13},
        {VNHumanHandPoseObservationJointNameRingPIP, 14},
        {VNHumanHandPoseObservationJointNameRingDIP, 15},
        {VNHumanHandPoseObservationJointNameRingTip, 16},
        {VNHumanHandPoseObservationJointNameLittleMCP, 17},
        {VNHumanHandPoseObservationJointNameLittlePIP, 18},
        {VNHumanHandPoseObservationJointNameLittleDIP, 19},
        {VNHumanHandPoseObservationJointNameLittleTip, 20},
    };
    int found = 0;
    float confAcc = 0.0f;
    for (const auto& [name, idx] : kMap) {
        VNRecognizedPoint* p = joints[name];
        if (p == nil || p.confidence < 0.2f) continue;
        // Vision: normalized, origin bottom-left -> flip y to the top-left
        // convention; mirror x like the preview.
        float x = (float) p.location.x;
        if (mirror) x = 1.0f - x;
        out.pt[(size_t) idx] = {x, 1.0f - (float) p.location.y};
        confAcc += (float) p.confidence;
        ++found;
    }
    // Accept when enough joints exist AND the mean joint confidence clears
    // the user's Confidence (MediaPipe's min_detection_confidence).
    if (found >= 15 && confAcc / (float) found >= minConfidence) {
        out.present = true;
        out.confidence = confAcc / (float) found;
    }
    return out.present;
}

int detectHandLandmarks(void* cvPixelBuffer, bool mirror, float minConfidence,
                        HandLandmarks* out, int maxHands) {
    for (int i = 0; i < maxHands; ++i) out[i] = HandLandmarks{};
    if (cvPixelBuffer == nullptr || maxHands < 1) return 0;
    if (@available(macOS 11.0, *)) {
        int landed = 0;
        @autoreleasepool {
            VNDetectHumanHandPoseRequest* req = [[VNDetectHumanHandPoseRequest alloc] init];
            req.maximumHandCount = (NSUInteger) maxHands;
            VNImageRequestHandler* handler = [[VNImageRequestHandler alloc]
                initWithCVPixelBuffer:(CVPixelBufferRef) cvPixelBuffer
                               options:@{}];
            NSError* err = nil;
            if ([handler performRequests:@[ req ] error:&err]) {
                for (VNHumanHandPoseObservation* obs in req.results) {
                    HandLandmarks lm;
                    if (!fillHand(obs, mirror, minConfidence, lm)) continue;
                    // Stable slots by chirality: the user's LEFT hand -> slot 0,
                    // RIGHT -> slot 1 (frames are captured unmirrored, so the
                    // reported chirality is the physical hand). Unknown or
                    // occupied slots spill into the first free one.
                    int slot = -1;
                    if (maxHands >= 2) {
                        if (@available(macOS 12.0, *)) {
                            if (obs.chirality == VNChiralityLeft) slot = 0;
                            else if (obs.chirality == VNChiralityRight) slot = 1;
                        }
                    }
                    if (slot < 0 || slot >= maxHands || out[slot].present)
                        for (int i = 0; i < maxHands; ++i)
                            if (!out[i].present) { slot = i; break; }
                    if (slot >= 0 && slot < maxHands && !out[slot].present) {
                        out[slot] = lm;
                        ++landed;
                    }
                    if (landed >= maxHands) break;
                }
            }
            [handler release];
            [req release];
        }
        return landed;
    }
    return 0;
}

// The camera path hands Vision a CVPixelBuffer; a still has to be wrapped in
// one first. Only the export uses this, so it copies rather than pools.
bool handsDetectorAvailable() {
    if (@available(macOS 11.0, *)) return true;
    return false;
}

bool handsSystemTracker() { return handsDetectorAvailable(); }

int detectHandLandmarks(const juce::Image& frame, bool mirror, float minConfidence,
                        HandLandmarks* out, int maxHands) {
    for (int i = 0; i < maxHands; ++i) out[i] = HandLandmarks{};
    if (!frame.isValid() || maxHands < 1) return 0;
    const int w = frame.getWidth(), h = frame.getHeight();
    CVPixelBufferRef pb = nullptr;
    NSDictionary* attrs = @{
        (id) kCVPixelBufferCGImageCompatibilityKey : @YES,
        (id) kCVPixelBufferCGBitmapContextCompatibilityKey : @YES,
    };
    if (CVPixelBufferCreate(kCFAllocatorDefault, (size_t) w, (size_t) h,
                            kCVPixelFormatType_32BGRA, (__bridge CFDictionaryRef) attrs,
                            &pb) != kCVReturnSuccess || pb == nullptr)
        return 0;
    CVPixelBufferLockBaseAddress(pb, 0);
    auto* base = (std::uint8_t*) CVPixelBufferGetBaseAddress(pb);
    const size_t stride = CVPixelBufferGetBytesPerRow(pb);
    {
        const juce::Image::BitmapData bd(frame, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < h; ++y) {
            auto* row = base + (size_t) y * stride;
            const auto* src = bd.getLinePointer(y);
            for (int x = 0; x < w; ++x, row += 4, src += bd.pixelStride) {
                if (bd.pixelFormat == juce::Image::ARGB) {
                    const auto* px = reinterpret_cast<const juce::PixelARGB*>(src);
                    row[0] = px->getBlue(); row[1] = px->getGreen(); row[2] = px->getRed();
                } else if (bd.pixelFormat == juce::Image::RGB) {
                    const auto* px = reinterpret_cast<const juce::PixelRGB*>(src);
                    row[0] = px->getBlue(); row[1] = px->getGreen(); row[2] = px->getRed();
                } else {
                    const juce::Colour c = bd.getPixelColour(x, y);
                    row[0] = c.getBlue(); row[1] = c.getGreen(); row[2] = c.getRed();
                }
                row[3] = 255;
            }
        }
    }
    CVPixelBufferUnlockBaseAddress(pb, 0);
    const int n = detectHandLandmarks((void*) pb, mirror, minConfidence, out, maxHands);
    CVPixelBufferRelease(pb);
    return n;
}

}  // namespace hum

#endif  // JUCE_MAC
