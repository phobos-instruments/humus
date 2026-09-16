// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/NativePicture.h"

#include <cstdint>

#if defined(__APPLE__)
#include <CoreVideo/CoreVideo.h>
#endif

namespace hum {

#if defined(__APPLE__)
bool lockNativePicture(void* native, NativePictureView& out) {
    out = {};
    if (native == nullptr) return false;
    auto pb = (CVPixelBufferRef) native;
    if (CVPixelBufferGetPixelFormatType(pb) != kCVPixelFormatType_32BGRA) return false;
    if (CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) return false;
    out.width = (int) CVPixelBufferGetWidth(pb);
    out.height = (int) CVPixelBufferGetHeight(pb);
    out.strideBytes = (int) CVPixelBufferGetBytesPerRow(pb);
    out.base = (const std::uint8_t*) CVPixelBufferGetBaseAddress(pb);
    out.bgra = true;
    if (out.base == nullptr || out.width <= 0 || out.height <= 0) {
        CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
        out = {};
        return false;
    }
    return true;
}

void unlockNativePicture(void* native) {
    if (native != nullptr) CVPixelBufferUnlockBaseAddress((CVPixelBufferRef) native, kCVPixelBufferLock_ReadOnly);
}
#else
bool lockNativePicture(void*, NativePictureView& out) {
    out = {};
    return false;
}

void unlockNativePicture(void*) {}
#endif

}
