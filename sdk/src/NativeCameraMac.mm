// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
// The AVCaptureSession video tap every camera consumer shares (Hands,
// Skeleton, CameraIn): frames arrive as CVPixelBuffers on the capture queue,
// retained into a FrameRef the consumer may keep. No photo output - JUCE's
// CameraDevice attaches one whose KVO wrapper logs "NSKVONotifying_
// AVCapturePhotoOutput not linked" at every open.
// Apple frameworks FIRST: AVFoundation drags in ApplicationServices/QuickDraw,
// whose legacy global `Pattern` type collides if our headers have already been
// parsed into the translation unit.
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <objc/runtime.h>

#include "hum/NativeCamera.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace hum {
namespace {

using FrameFn = std::function<void(const NativeCamera::FrameRef&)>;

// The capture delegate is a RUNTIME-built ObjC class with a per-image unique
// name: this static lib links into the app, the dyn pack dylibs AND the
// plugin, and a compile-time @interface would register the same class name in
// every image (objc then warns about "spurious casting failures"). The ivar
// holds a pointer to the owning camera's std::function.
void captureIMP(id self, SEL, AVCaptureOutput*, CMSampleBufferRef sampleBuffer,
                AVCaptureConnection*) {
    Ivar iv = class_getInstanceVariable(object_getClass(self), "humOnFrame");
    if (iv == nullptr) return;
    auto* fn =
        (FrameFn*) *(void**) ((char*) (__bridge void*) self + ivar_getOffset(iv));
    if (fn == nullptr || !*fn) return;
    CVImageBufferRef pb = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (pb == nullptr) return;
    NativeCamera::FrameRef f;
    f.buffer = (void*) pb;
    f.width = (int) CVPixelBufferGetWidth(pb);
    f.height = (int) CVPixelBufferGetHeight(pb);
    CVBufferRetain(pb);
    f.hold = std::shared_ptr<const void>(
        (const void*) pb, [](const void* p) { CVBufferRelease((CVPixelBufferRef) p); });
    (*fn)(f);
}

Class delegateClass() {
    static Class cls = [] {
        char name[64];
        std::snprintf(name, sizeof(name), "HumNativeCameraDelegate_%p",
                      (void*) &captureIMP);   // unique per loaded image
        Class c = objc_allocateClassPair([NSObject class], name, 0);
        class_addIvar(c, "humOnFrame", sizeof(void*),
                      (uint8_t) std::log2(sizeof(void*)), @encode(void*));
        class_addMethod(c, @selector(captureOutput:didOutputSampleBuffer:fromConnection:),
                        (IMP) captureIMP, "v@:@@@");
        if (Protocol* proto = @protocol(AVCaptureVideoDataOutputSampleBufferDelegate))
            class_addProtocol(c, proto);
        objc_registerClassPair(c);
        return c;
    }();
    return cls;
}

id makeDelegate(FrameFn* fn) {
    id obj = [[delegateClass() alloc] init];
    Ivar iv = class_getInstanceVariable(object_getClass(obj), "humOnFrame");
    if (iv != nullptr)
        *(void**) ((char*) (__bridge void*) obj + ivar_getOffset(iv)) = fn;
    return obj;
}

// The device the user picked, matched by localizedName - the same string
// juce::CameraDevice::getAvailableDevices() lists (and the same discovery
// set: built-in + external/Continuity cameras). No match (unplugged) or an
// empty name falls back to the system default.
AVCaptureDevice* deviceNamed(const std::string& name) {
    if (!name.empty()) {
        NSString* want = [NSString stringWithUTF8String:name.c_str()];
        if (@available(macOS 10.15, *)) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            AVCaptureDeviceDiscoverySession* discovery = [AVCaptureDeviceDiscoverySession
                discoverySessionWithDeviceTypes:@[
                    AVCaptureDeviceTypeBuiltInWideAngleCamera,
                    AVCaptureDeviceTypeExternalUnknown
                ]
                                      mediaType:AVMediaTypeVideo
                                       position:AVCaptureDevicePositionUnspecified];
#pragma clang diagnostic pop
            for (AVCaptureDevice* d in discovery.devices)
                if ([d.localizedName isEqualToString:want]) return d;
        }
    }
    return [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
}

class MacNativeCamera final : public NativeCamera {
public:
    bool start(const std::string& deviceName, FrameFn onFrame) {
        onFrame_ = std::move(onFrame);
        delegate_ = makeDelegate(&onFrame_);
        session_ = [[AVCaptureSession alloc] init];
        AVCaptureDevice* dev = deviceNamed(deviceName);
        if (dev == nil) return false;
        NSError* err = nil;
        AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:dev
                                                                            error:&err];
        if (input == nil || ![session_ canAddInput:input]) return false;
        [session_ addInput:input];
        // 720p when the device offers it - trackers get more pixels per joint
        // and previews stop looking pixelated; 640x480 is the floor.
        if ([session_ canSetSessionPreset:AVCaptureSessionPreset1280x720])
            session_.sessionPreset = AVCaptureSessionPreset1280x720;
        else if ([session_ canSetSessionPreset:AVCaptureSessionPreset640x480])
            session_.sessionPreset = AVCaptureSessionPreset640x480;

        AVCaptureVideoDataOutput* out = [[AVCaptureVideoDataOutput alloc] init];
        out.videoSettings =
            @{(id) kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};
        out.alwaysDiscardsLateVideoFrames = YES;   // live wants fresh, not complete
        queue_ = dispatch_queue_create("hum.native.camera", DISPATCH_QUEUE_SERIAL);
        [out setSampleBufferDelegate:delegate_ queue:queue_];
        const bool ok = [session_ canAddOutput:out];
        if (ok) [session_ addOutput:out];
        [out release];
        if (!ok) return false;
        [session_ startRunning];
        return true;
    }

    ~MacNativeCamera() override {
        if (session_ != nil) [session_ stopRunning];
        if (queue_ != nullptr) dispatch_sync(queue_, ^{});   // drain in-flight callback
        if (delegate_ != nil) [delegate_ release];
        onFrame_ = nullptr;   // after the queue drained: no callback can race
        if (session_ != nil) [session_ release];
        if (queue_ != nullptr) dispatch_release(queue_);
    }

private:
    AVCaptureSession* session_ = nil;
    id delegate_ = nil;
    dispatch_queue_t queue_ = nullptr;
    FrameFn onFrame_;
};

}   // namespace

std::unique_ptr<NativeCamera> NativeCamera::open(
    const std::string& deviceName, std::function<void(const FrameRef&)> onFrame) {
    const AVAuthorizationStatus st =
        [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (st == AVAuthorizationStatusNotDetermined) {
        // Ask once; the caller's lifecycle poll retries after the user answers.
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                                 completionHandler:^(BOOL) {}];
        return nullptr;
    }
    if (st != AVAuthorizationStatusAuthorized) return nullptr;
    auto cam = std::make_unique<MacNativeCamera>();
    if (!cam->start(deviceName, std::move(onFrame))) return nullptr;
    return cam;
}

bool NativeCamera::copyRgba(const FrameRef& frame, bool mirror,
                            std::vector<std::uint8_t>& rgba, int& width, int& height) {
    auto* pb = (CVPixelBufferRef) frame.buffer;
    if (pb == nullptr) return false;
    if (CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly)
        != kCVReturnSuccess)
        return false;
    const int w = (int) CVPixelBufferGetWidth(pb);
    const int h = (int) CVPixelBufferGetHeight(pb);
    const auto stride = (size_t) CVPixelBufferGetBytesPerRow(pb);
    const auto* base = (const std::uint8_t*) CVPixelBufferGetBaseAddress(pb);
    const bool ok = base != nullptr && w > 0 && h > 0;
    if (ok) {
        width = w;
        height = h;
        rgba.resize((size_t) w * (size_t) h * 4);
        for (int y = 0; y < h; ++y) {
            const auto* row = base + (size_t) y * stride;
            auto* out = rgba.data() + (size_t) y * (size_t) w * 4;
            for (int x = 0; x < w; ++x, out += 4) {
                const auto* q = row + (size_t) (mirror ? w - 1 - x : x) * 4;
                out[0] = q[2];
                out[1] = q[1];
                out[2] = q[0];
                out[3] = 255;
            }
        }
    }
    CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
    return ok;
}

}   // namespace hum
