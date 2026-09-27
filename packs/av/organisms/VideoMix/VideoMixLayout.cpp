// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include <string>
#include <vector>

#include "VideoMix/VideoMix.h"
#include "hum/LayoutSpec.h"
#include "hum/ParseInt.h"
#include "hum/Registry.h"

namespace hum {
namespace {

constexpr int kPad = 12, kHeaderH = 20, kFaderTop = 34, kFaderH = 120, kFaderW = 40, kHalfRackMost = 4;

int channelsOf(const std::string& cls) {
    const std::string stem = "VideoMix";
    if (cls.rfind(stem, 0) != 0 || cls.size() == stem.size()) return 0;
    const int n = parseBoundedInt(cls.substr(stem.size()));
    return n >= 3 && n <= kVideoMixMostChannels ? n : 0;
}

LayoutSpec channelMixLayout(int n) {
    using CT = LayoutSpec::ControlType;
    LayoutSpec spec;
    spec.width = n <= kHalfRackMost ? kRackHalfW : kRackFullW;
    std::vector<std::string> sizes;
    for (int k = 2; k <= kVideoMixMostChannels; ++k) sizes.push_back(std::to_string(k));
    spec.controls.push_back({CT::Combo, "", "", "Inputs", kPad, 6, 108, kHeaderH, 2, false,
                             std::move(sizes), {{"reclass", "inputs"}}});
    const int pitch = (spec.width - 2 * kPad) / n;
    for (int k = 1; k <= n; ++k) {
        const int x = kPad + (k - 1) * pitch + (pitch - kFaderW) / 2;
        spec.controls.push_back({CT::VSlider, "Level_" + std::to_string(k), "", std::to_string(k),
                                 x, kFaderTop, kFaderW, kFaderH, 2, false});
    }
    spec.height = kFaderTop + kFaderH + 8;
    return spec;
}

}

void hum_register_layouts_av(Registry& r) {
    r.registerLayoutProvider("av", [](const std::string& genId, const std::string& cls) -> std::string {
        if (genId != "videomix" && !genId.empty()) return {};
        const int n = channelsOf(cls);
        return n > 0 ? toJson(channelMixLayout(n)) : std::string();
    });
}

}
