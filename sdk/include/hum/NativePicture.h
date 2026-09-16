// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>

namespace hum {

struct NativePictureView {
    int width = 0;
    int height = 0;
    int strideBytes = 0;
    const std::uint8_t* base = nullptr;
    bool bgra = true;
};

bool lockNativePicture(void* native, NativePictureView& out);
void unlockNativePicture(void* native);

}
