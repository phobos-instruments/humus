// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

extern "C" {
#include <libavutil/hwcontext.h>
}

namespace hum::video {

inline bool hardwareWanted(const juce::String& setting) {
    return !(setting.equalsIgnoreCase("off") || setting == "0");
}

inline std::vector<AVHWDeviceType> decoderTries(bool hardware) {
    std::vector<AVHWDeviceType> tries;
    if (hardware) {
#if defined(_WIN32)
        tries.push_back(AV_HWDEVICE_TYPE_D3D11VA);
#endif
        tries.push_back(AV_HWDEVICE_TYPE_CUDA);
        tries.push_back(AV_HWDEVICE_TYPE_VAAPI);
    }
    tries.push_back(AV_HWDEVICE_TYPE_NONE);
    return tries;
}

}
